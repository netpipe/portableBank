#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QDateTime>
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QRandomGenerator>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDebug>

void resetExpiredTokens() {
    // Get current time
    QDateTime currentTime = QDateTime::currentDateTime();

    // Query expired tokens that haven't been redeemed
    QSqlQuery query;
    query.prepare("SELECT token FROM valid_tokens WHERE redeemed = 0 AND token_expiry < :currentTime");
    query.bindValue(":currentTime", currentTime.toString(Qt::ISODate));

    if (!query.exec()) {
        qDebug() << "Failed to query expired tokens.";
        return;
    }

    QStringList expiredTokens;
    while (query.next()) {
        expiredTokens << query.value(0).toString();
    }

    // Reset tokens
    if (!expiredTokens.isEmpty()) {
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE valid_tokens SET redeemed = 0 WHERE token IN (:tokens)");
        updateQuery.bindValue(":tokens", expiredTokens.join(","));
        if (updateQuery.exec()) {
            qDebug() << "Successfully reset expired tokens.";
        } else {
            qDebug() << "Failed to reset expired tokens.";
        }
    } else {
        qDebug() << "No expired tokens to reset.";
    }
}

QString generateRandomToken(int length = 12) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString token;
    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(chars.length());
        token += chars.at(index);
    }
    return token;
}

void bruteForceTokenPool2(int total = 100000) {
    QSqlQuery clear;
    clear.exec("DELETE FROM all_tokens");

    QSqlQuery insert;
    insert.prepare("INSERT OR IGNORE INTO all_tokens (token) VALUES (:token)");

    int inserted = 0;
    while (inserted < total) {
        QString token = generateRandomToken(12);
        insert.bindValue(":token", token);
        insert.exec();

        if (insert.numRowsAffected() > 0) {
            inserted++;
        }
    }
}

void bruteForceTokenPool(int total = 100000, int batchSize = 1000) {
    QSqlQuery clear;
    clear.exec("DELETE FROM all_tokens");

    QSqlDatabase::database().transaction();

    QSet<QString> uniqueTokens;
    while (uniqueTokens.size() < total) {
        uniqueTokens.insert(generateRandomToken(12));
    }

    QStringList tokens = uniqueTokens.values();
    QSqlQuery insert;
    for (int i = 0; i < tokens.size(); i += batchSize) {
        QStringList batch;
        for (int j = i; j < i + batchSize && j < tokens.size(); ++j) {
            batch << QString("('%1')").arg(tokens[j]);
        }
        QString sql = "INSERT OR IGNORE INTO all_tokens (token) VALUES " + batch.join(", ");
        insert.exec(sql);
    }

    QSqlDatabase::database().commit();
}

void selectValidTokens(int count = 1000) {
    QSqlQuery q;
    q.exec("DELETE FROM valid_tokens");
    QSqlQuery insert;
    insert.prepare("INSERT INTO valid_tokens (token) SELECT token FROM all_tokens ORDER BY RANDOM() LIMIT :count");
    insert.bindValue(":count", count);
    insert.exec();
}

QString validateTokenRedemption(const QString &token) {
    QSqlQuery check;
    check.prepare("SELECT 1 FROM all_tokens WHERE token = :token");
    check.bindValue(":token", token);
    if (!check.exec() || !check.next()) {
        return "Invalid Token: Not recognized (forged).";
    }

    QSqlQuery valid;
    valid.prepare("SELECT redeemed FROM valid_tokens WHERE token = :token");
    valid.bindValue(":token", token);
    if (!valid.exec() || !valid.next()) {
        return "Invalid Token: Not part of active set.";
    }

    if (valid.value(0).toBool()) {
        return "Token already redeemed.";
    }

    QSqlQuery update;
    update.prepare("UPDATE valid_tokens SET redeemed = 1 WHERE token = :token");
    update.bindValue(":token", token);
    update.exec();
    return "Token successfully redeemed.";
}

QString generateTokenFile(int count = 10, int hoursToExpire = 24) {
    QSqlQuery query;
    query.prepare("SELECT token FROM valid_tokens WHERE redeemed = 0 LIMIT :count");
    query.bindValue(":count", count);
    query.exec();

    QStringList tokens;
    while (query.next()) {
        tokens << query.value(0).toString();
    }

    QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString expiry = QDateTime::currentDateTime().addSecs(hoursToExpire * 3600).toString(Qt::ISODate);

    QStringList lines;
    lines << "# IOU Export File";
    lines << "Timestamp: " + timestamp;
    lines << "Expires: " + expiry;
    lines << "Tokens:";
    for (const QString &t : tokens) lines << t;

    QString rawData = lines.join("\n");
    QByteArray md5 = QCryptographicHash::hash(rawData.toUtf8(), QCryptographicHash::Md5).toHex();
    lines << "MD5: " + QString(md5);

    // Save the original file
    QString filePath = QFileDialog::getSaveFileName(nullptr, "Export Token File", "", "Token File (*.iou)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << lines.join("\n");
            file.close();

            // Create a backup of the file in a different location or naming convention
            QString backupPath = filePath + ".bak";  // Example backup path
            QFile::copy(filePath, backupPath);  // Copy the file as a backup

            // Store backup file details in the database
            QSqlQuery insertFile;
            insertFile.prepare("INSERT INTO transaction_files (file_path, backup_path, expiry_time) VALUES (:file_path, :backup_path, :expiry_time)");
            insertFile.bindValue(":file_path", filePath);
            insertFile.bindValue(":backup_path", backupPath);
            insertFile.bindValue(":expiry_time", expiry);
            insertFile.exec();

            return QString("Exported %1 tokens to %2").arg(tokens.size()).arg(filePath);
        }
    }
    return "Export cancelled or failed.";
}

QString importTokenFile() {
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Import Token File", "", "Token File (*.iou)");
    if (filePath.isEmpty()) return "Import cancelled.";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "Failed to open file.";

    QStringList lines;
    while (!file.atEnd()) lines << file.readLine().trimmed();

    int md5Index = lines.lastIndexOf(QRegExp("^MD5: .*"));
    if (md5Index == -1) return "Invalid file format: No MD5 found.";

    QString originalMD5 = lines[md5Index].mid(5).trimmed();
    QString rawData = lines.mid(0, md5Index).join("\n");
    QByteArray calculatedMD5 = QCryptographicHash::hash(rawData.toUtf8(), QCryptographicHash::Md5).toHex();
    if (originalMD5 != calculatedMD5) return "MD5 mismatch: File integrity compromised.";

    QString expiryLine = lines.filter(QRegExp("^Expires:")).value(0);
    QDateTime expiry = QDateTime::fromString(expiryLine.mid(8).trimmed(), Qt::ISODate);
   // if (expiry.isValid() && expiry < QDateTime::currentDateTime()) {
   //     return "Token file expired on " + expiry.toString();
  //  }

    //maybe keep copies of the tx files given out with expirey so that it can automatically add them back
    if (expiry.isValid() && expiry < QDateTime::currentDateTime()) {
        // Optional: mark tokens as unused if they weren't redeemed
        int tokenStart = lines.indexOf("Tokens:") + 1;
        int tokenEnd = md5Index;
        QStringList expiredTokens = lines.mid(tokenStart, tokenEnd - tokenStart);

        for (const QString &token : expiredTokens) {
            QSqlQuery q;
            q.prepare("UPDATE valid_tokens SET redeemed = 0 WHERE token = :token");
            q.bindValue(":token", token);
            q.exec();
        }

        return "Token file expired and tokens were reset to unredeemed.";
    }


    int tokenStart = lines.indexOf("Tokens:") + 1;
    int tokenEnd = md5Index;
    QStringList importedTokens = lines.mid(tokenStart, tokenEnd - tokenStart);

    int redeemed = 0;
    for (const QString &token : importedTokens) {
        QSqlQuery q;
        q.prepare("UPDATE valid_tokens SET redeemed = 1 WHERE token = :token AND redeemed = 0");
        q.bindValue(":token", token);
        q.exec();
        if (q.numRowsAffected() > 0) redeemed++;
    }

    // all need to be valid to get proper cashout
    return QString("Imported %1 tokens. %2 were valid and redeemed.").arg(importedTokens.size()).arg(redeemed);
}

void restoreExpiredTokensFromBackup() {
    QSqlQuery query;
    query.prepare("SELECT backup_path, expiry_time FROM transaction_files WHERE expiry_time < :current_time");
    query.bindValue(":current_time", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        qDebug() << "Failed to retrieve expired transaction files.";
        return;
    }

    while (query.next()) {
        QString backupPath = query.value(0).toString();
        QDateTime expiry = QDateTime::fromString(query.value(1).toString(), Qt::ISODate);

        if (expiry < QDateTime::currentDateTime()) {
            // Restore tokens from backup file
            QFile backupFile(backupPath);
            if (backupFile.exists() && backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&backupFile);
                QStringList lines;
                while (!in.atEnd()) {
                    lines << in.readLine().trimmed();
                }

                int tokenStart = lines.indexOf("Tokens:") + 1;
                int tokenEnd = lines.indexOf("MD5:") - 1;
                QStringList tokensToRestore = lines.mid(tokenStart, tokenEnd - tokenStart);

                for (const QString &token : tokensToRestore) {
                    // Mark tokens as unredeemed
                    QSqlQuery q;
                    q.prepare("UPDATE valid_tokens SET redeemed = 0 WHERE token = :token");
                    q.bindValue(":token", token);
                    q.exec();
                }
                backupFile.close();
            }
        }
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Secure IOU Token System");
    auto *layout = new QVBoxLayout(&window);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("tokens.db");
    db.open();

    QSqlQuery init;
    init.exec("CREATE TABLE IF NOT EXISTS all_tokens (token TEXT PRIMARY KEY, token_expiry DATETIME)");
    init.exec("CREATE TABLE IF NOT EXISTS valid_tokens (token TEXT PRIMARY KEY, redeemed INTEGER DEFAULT 0, token_expiry DATETIME, FOREIGN KEY(token) REFERENCES all_tokens(token))");
    init.exec("CREATE TABLE IF NOT EXISTS transaction_files (file_path TEXT, backup_path TEXT, expiry_time DATETIME)");

    // Reset expired tokens at app startup
    resetExpiredTokens();

    // Check and restore tokens from expired files
    restoreExpiredTokensFromBackup();

    auto *output = new QPlainTextEdit;
    output->setReadOnly(true);
    auto *tokenInput = new QLineEdit;
    tokenInput->setPlaceholderText("Enter token to redeem...");

    auto *redeemBtn = new QPushButton("Redeem Token");
    auto *genAllBtn = new QPushButton("Generate All Tokens");
    auto *genValidBtn = new QPushButton("Select Valid Tokens");
    auto *exportBtn = new QPushButton("Export Tokens to File");
    auto *importBtn = new QPushButton("Import & Redeem Token File");

    layout->addWidget(tokenInput);
    layout->addWidget(redeemBtn);
    layout->addWidget(genAllBtn);
    layout->addWidget(genValidBtn);
    layout->addWidget(exportBtn);
    layout->addWidget(importBtn);
    layout->addWidget(output);

    QObject::connect(redeemBtn, &QPushButton::clicked, [&]() {
        QString result = validateTokenRedemption(tokenInput->text().trimmed());
        output->appendPlainText(result);
    });
    QObject::connect(genAllBtn, &QPushButton::clicked, [&]() {
        bruteForceTokenPool();
        output->appendPlainText("Generated all tokens.");
    });
    QObject::connect(genValidBtn, &QPushButton::clicked, [&]() {
        selectValidTokens();
        output->appendPlainText("Selected valid tokens.");
    });
    QObject::connect(exportBtn, &QPushButton::clicked, [&]() {
        output->appendPlainText(generateTokenFile());
    });
    QObject::connect(importBtn, &QPushButton::clicked, [&]() {
        output->appendPlainText(importTokenFile());
    });

    window.resize(500, 500);
    window.show();
    return app.exec();
}
