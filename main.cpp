#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>

class Bank : public QWidget {
    Q_OBJECT
public:
    Bank(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Banking App");
        setFixedSize(600, 500);

        QVBoxLayout *layout = new QVBoxLayout(this);

        searchInput = new QLineEdit(this);
        searchInput->setPlaceholderText("Search Client");
        layout->addWidget(searchInput);
        connect(searchInput, &QLineEdit::textChanged, this, &Bank::searchClients);

        clientTable = new QTableWidget(this);
        clientTable->setColumnCount(4);
        clientTable->setHorizontalHeaderLabels({"Name", "Address", "Balance", "Loan"});
        clientTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        connect(clientTable, &QTableWidget::cellClicked, this, &Bank::selectClient);
        layout->addWidget(clientTable);

        nameInput = new QLineEdit(this);
        nameInput->setPlaceholderText("Enter Name");
        layout->addWidget(nameInput);

        addressInput = new QLineEdit(this);
        addressInput->setPlaceholderText("Enter Address");
        layout->addWidget(addressInput);

        amountInput = new QLineEdit(this);
        amountInput->setPlaceholderText("Enter Amount");
        layout->addWidget(amountInput);

        QPushButton *createButton = new QPushButton("Create Account", this);
        connect(createButton, &QPushButton::clicked, this, &Bank::createAccount);
        layout->addWidget(createButton);

        QPushButton *depositButton = new QPushButton("Deposit", this);
        connect(depositButton, &QPushButton::clicked, this, &Bank::deposit);
        layout->addWidget(depositButton);

        QPushButton *loanButton = new QPushButton("Take Loan", this);
        connect(loanButton, &QPushButton::clicked, this, &Bank::takeLoan);
        layout->addWidget(loanButton);

        QPushButton *historyButton = new QPushButton("Show Transaction History", this);
        connect(historyButton, &QPushButton::clicked, this, &Bank::displayHistory);
        layout->addWidget(historyButton);

        output = new QTextEdit(this);
        output->setReadOnly(true);
        layout->addWidget(output);

        db = QSqlDatabase::addDatabase("QSQLITE");
#ifdef __APPLE__
                db.setDatabaseName("/Applications/portableBank.app/Contents/MacOS/bank.db");
#else
        db.setDatabaseName("bank.db");
#endif
        if (!db.open()) {
            qDebug() << "Error: Connection failed!";
            return;
        }
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, address TEXT, balance REAL, loan REAL)");
        query.exec("CREATE TABLE IF NOT EXISTS transactions (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, type TEXT, amount REAL, date TEXT)");
        loadClients();
    }

public slots:
    void createAccount() {
        QString name = nameInput->text();
        QString address = addressInput->text();
        double initialDeposit = amountInput->text().toDouble();
        QSqlQuery query;
        query.prepare("INSERT INTO accounts (name, address, balance, loan) VALUES (:name, :address, :balance, :loan)");
        query.bindValue(":name", name);
        query.bindValue(":address", address);
        query.bindValue(":balance", initialDeposit);
        query.bindValue(":loan", 0.0);
        query.exec();

        if (initialDeposit > 0) {
            logTransaction(name, "Deposit", initialDeposit);
        }
        loadClients();
        output->append("Account created for " + name);
    }

    void deposit() {
        QString name = nameInput->text();
        double amount = amountInput->text().toDouble();
        if (amountInput->text() != "0" ){
        QSqlQuery query;
        query.prepare("UPDATE accounts SET balance = balance + :amount WHERE name = :name");
        query.bindValue(":amount", amount);
        query.bindValue(":name", name);
        query.exec();

        logTransaction(name, "Deposit", amount);
        loadClients();
        output->append("Deposited " + QString::number(amount) + " to " + name);
        }
    }

    void takeLoan() {
        QString name = nameInput->text();
        double amount = amountInput->text().toDouble();
        if (amountInput->text() != "0" ){
        QSqlQuery query;
        query.prepare("UPDATE accounts SET loan = loan + :amount, balance = balance + :amount WHERE name = :name");
        query.bindValue(":amount", amount);
        query.bindValue(":name", name);
        query.exec();

        logTransaction(name, "Loan", amount);
        loadClients();
        output->append(name + " took a loan of " + QString::number(amount));
        }
    }

    void displayHistory() {
        output->clear();
        QString name = nameInput->text();
        QSqlQuery query;
        query.prepare("SELECT type, amount, date FROM transactions WHERE name = :name");
        query.bindValue(":name", name);
        query.exec();

        output->append("Transaction History for " + name + ":");
        while (query.next()) {
            output->append("Type: " + query.value(0).toString() + " Amount: " + QString::number(query.value(1).toDouble()) + " Date: " + query.value(2).toString());
        }
    }

    void logTransaction(const QString &name, const QString &type, double amount) {
        QSqlQuery query;
        query.prepare("INSERT INTO transactions (name, type, amount, date) VALUES (:name, :type, :amount, datetime('now'))");
        query.bindValue(":name", name);
        query.bindValue(":type", type);
        query.bindValue(":amount", amount);
        query.exec();
    }

    void loadClients() {
        clientTable->setRowCount(0);
        QSqlQuery query("SELECT name, address, balance, loan FROM accounts");
        int row = 0;
        while (query.next()) {
            clientTable->insertRow(row);
            clientTable->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
            clientTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
            clientTable->setItem(row, 2, new QTableWidgetItem(QString::number(query.value(2).toDouble())));
            clientTable->setItem(row, 3, new QTableWidgetItem(QString::number(query.value(3).toDouble())));
            row++;
        }
    }
    void calculateYearlyInterest() {
        double rate = 5.0;
        QSqlQuery query("SELECT id, balance FROM accounts");
        while (query.next()) {
            int id = query.value(0).toInt();
            double balance = query.value(1).toDouble();
            double interest = balance * (rate / 100.0);
            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE accounts SET balance = balance + :interest WHERE id = :id");
            updateQuery.bindValue(":interest", interest);
            updateQuery.bindValue(":id", id);
            updateQuery.exec();
        }
        output->append("Yearly interest applied.");
    }

    void applyDailyInterest() {
        double annualInterestRate = 5.0; // 5% yearly interest
        double dailyInterestRate = annualInterestRate / 365.0 / 100.0;

        QSqlQuery query("SELECT id, balance, loan FROM accounts");
        while (query.next()) {
            int id = query.value(0).toInt();
            double balance = query.value(1).toDouble();
            double loan = query.value(2).toDouble();

            double balanceInterest = balance * dailyInterestRate * 30; // Assuming 30 days in a month
            double loanInterest = loan * dailyInterestRate * 30;

            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE accounts SET balance = balance + :balanceInterest, loan = loan + :loanInterest WHERE id = :id");
            updateQuery.bindValue(":balanceInterest", balanceInterest);
            updateQuery.bindValue(":loanInterest", loanInterest);
            updateQuery.bindValue(":id", id);
            updateQuery.exec();

            logTransaction("System", "Interest Applied", balanceInterest - loanInterest);
        }
    }

    void searchClients() {
        QString searchText = searchInput->text();
        for (int i = 0; i < clientTable->rowCount(); i++) {
            bool match = clientTable->item(i, 0)->text().contains(searchText, Qt::CaseInsensitive);
            clientTable->setRowHidden(i, !match);
        }
    }

    void selectClient(int row, int) {
        nameInput->setText(clientTable->item(row, 0)->text());
        addressInput->setText(clientTable->item(row, 1)->text());
        amountInput->setText("0");
    }

private:
    QSqlDatabase db;
    QLineEdit *nameInput;
    QLineEdit *addressInput;
    QLineEdit *amountInput;
    QLineEdit *searchInput;
    QTextEdit *output;
    QTableWidget *clientTable;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Bank bank;
    bank.show();
    return app.exec();
}

#include "main.moc"
