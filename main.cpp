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
#include <QDate>
#include <QMessageBox>
#include <qtimer.h>
#include <QSplitter>

bool bmidnight;
bool intrestbuttont;
bool btoggle;

class Bank : public QWidget {
    Q_OBJECT
public:
    Bank(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Banking App");
        setFixedSize(700, 500);

        QVBoxLayout *layout = new QVBoxLayout(this);

        QSplitter *splitter = new QSplitter(parent);
        QSplitter *splitter2 = new QSplitter(parent);
        QSplitter *splitter3 = new QSplitter(parent);

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
        clientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        nameInput = new QLineEdit(this);
        nameInput->setPlaceholderText("Enter Name");
        layout->addWidget(nameInput);

        addressInput = new QLineEdit(this);
        addressInput->setPlaceholderText("Enter Address");
        layout->addWidget(addressInput);

        amountInput = new QLineEdit(this);
        amountInput->setPlaceholderText("Enter Amount");
        layout->addWidget(amountInput);

        QPushButton *depositButton = new QPushButton("Deposit", this);
        connect(depositButton, &QPushButton::clicked, this, &Bank::deposit);
                splitter->addWidget(depositButton);


        QPushButton *loanButton = new QPushButton("Take Loan", this);
        connect(loanButton, &QPushButton::clicked, this, &Bank::takeLoan);
        splitter->addWidget(loanButton);


        QPushButton *historyButton = new QPushButton("Show History", this);
        connect(historyButton, &QPushButton::clicked, this, &Bank::displayHistory);
        splitter->addWidget(historyButton);
        layout->addWidget(splitter);


        QLabel *intrestlabel = new QLabel(this);
        intrestlabel->setText("Yearly Intrest Rate %");
        splitter3->addWidget(intrestlabel);

        amountIntrest = new QLineEdit(this);
        amountIntrest->setPlaceholderText("yearly intrest 5%");
        amountIntrest->setText("5");
        splitter3->addWidget(amountIntrest);

        QPushButton *interestButton = new QPushButton("ApplyDailyInterest", this);
        connect(interestButton, &QPushButton::clicked, this, &Bank::applyDailyInterestButton);
        splitter3->addWidget(interestButton);
       layout->addWidget(splitter3);

       QPushButton *createButton = new QPushButton("Create Account", this);
       connect(createButton, &QPushButton::clicked, this, &Bank::createAccount);
       splitter2->addWidget(createButton);

        QPushButton *editButton = new QPushButton("Edit Address", this);
        connect(editButton, &QPushButton::clicked, this, &Bank::StoreChanges);
        splitter2->addWidget(editButton);

        QPushButton *deleteButton = new QPushButton("Delete Account", this);
        connect(deleteButton, &QPushButton::clicked, this, &Bank::deleteAccount);
        splitter2->addWidget(deleteButton);
        layout->addWidget(splitter2);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, QOverload<>::of(&Bank::CheckTime));
        //timer->start(10000);

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Question", "would you like the program to apply daily intrest updates for you ?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            timer->start(30000);
            intrestbuttont=false;
            applyDailyInterest();
        }

        output = new QTextEdit(this);
        output->setReadOnly(true);
        layout->addWidget(output);

        db = QSqlDatabase::addDatabase("QSQLITE");
#ifdef __A //PPLE__
                db.setDatabaseName("/Applications/portableBank.app/Contents/MacOS/bank.db");
#else
        db.setDatabaseName(QCoreApplication::applicationDirPath() +"/bank.db");
#endif
        if (!db.open()) {
            qDebug() << "Error: Connection failed!";
            return;
        }
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS accounts (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, address TEXT, balance REAL, loan REAL, last_interest_date TEXT)");
        query.exec("CREATE TABLE IF NOT EXISTS transactions (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, type TEXT, amount REAL, date TEXT)");
        loadClients();
    }

public slots:
    void editToggle() {
        if ( !btoggle ) {
         clientTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
         btoggle=true;
        }else{
         clientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
         btoggle=false;
        }
    }
    void StoreChanges() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "are you sure", "update address ?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
        QSqlQuery query;
        query.prepare("UPDATE accounts SET address = :address WHERE name = :name");
        query.bindValue(":address", addressInput->text());
        query.bindValue(":name", nameInput->text());
       //clientTable->currentRow()
        query.exec();
       // logTransaction(name, "Deposit", amount);
        loadClients();
        }
    }



    void createAccount() {     

        //look for doubles before making second
        QString name = nameInput->text();
        QString address = addressInput->text();
        double initialDeposit = amountInput->text().toDouble();
                    QDate currentDate = QDate::currentDate();
        QSqlQuery query;
        query.prepare("INSERT INTO accounts (name, address, balance, loan, last_interest_date) VALUES (:name, :address, :balance, :loan, :intrestdate)");
        query.bindValue(":name", name);
        query.bindValue(":address", address);
        query.bindValue(":balance", initialDeposit);
        query.bindValue(":loan", 0.0);
        query.bindValue(":intrestdate", currentDate.toString("yyyy-MM-dd"));
        query.exec();

        if (initialDeposit > 0) {
            logTransaction(name, "Deposit", initialDeposit);
        }
        loadClients();
        output->append("Account created for " + name);
    }

     void CheckTime() {
         QTimer *timer;

         QTime currentTime = QTime::currentTime();
       if( currentTime.hour() == 24 && currentTime.minute() < 2){
           intrestbuttont=false;
           bmidnight=1;
           applyDailyInterest();
       }else{
           bmidnight=0;
       }
        qDebug() << currentTime.hour();
     }

    void deleteAccount() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "are you sure", "Delete Account ?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {


        QString name = nameInput->text();
        QSqlQuery query;
        query.prepare("DELETE FROM accounts WHERE name = :name");
        query.bindValue(":name", name);
        query.exec();

        logTransaction("System", name+" Removed", 0);
           loadClients();
        }

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

   void applyDailyInterestButton() {
  intrestbuttont=true;
        applyDailyInterest();
    }

    void applyDailyInterest() {
        QTime currentDate = QTime::currentTime();


        QMessageBox::StandardButton reply;
        if (intrestbuttont ) {
        reply = QMessageBox::question(this, "are you sure", "Apply Intrest ?",
                                      QMessageBox::Yes|QMessageBox::No);
        }else {intrestbuttont=false;
            reply = QMessageBox::Yes;
        qDebug() << "button intrest" ;
        }


        if (reply == QMessageBox::Yes) {


        double annualInterestRate = amountIntrest->text().toInt(); // 5% yearly interest
        double dailyInterestRate = annualInterestRate / 365.0 / 100.0;

        QSqlQuery query("SELECT id, balance, loan, last_interest_date FROM accounts");
        while (query.next()) {
            int id = query.value(0).toInt();
            double balance = query.value(1).toDouble();
            double loan = query.value(2).toDouble();
            QString lastInterestDate = query.value(3).toString();

            // Get the current date
            QDate currentDate = QDate::currentDate();
            QDate lastDate = QDate::fromString(lastInterestDate, "yyyy-MM-dd");

            // If last interest date is not set, assume account was created today
            if (!lastDate.isValid()) {
                lastDate = currentDate;
            }

            int daysElapsed = lastDate.daysTo(currentDate);
            qDebug() << daysElapsed << endl;
            if (daysElapsed <= 0) continue;

            double balanceInterest = balance * dailyInterestRate * daysElapsed;
            double loanInterest = loan * dailyInterestRate * daysElapsed;
 qDebug() << "balance intrest" << balanceInterest << endl;
  qDebug() << loanInterest << endl;
            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE accounts SET balance = balance + :balanceInterest, loan = loan + :loanInterest, last_interest_date = :newDate WHERE id = :id");
            updateQuery.bindValue(":balanceInterest", balanceInterest);
            updateQuery.bindValue(":loanInterest", loanInterest);
            updateQuery.bindValue(":newDate", currentDate.toString("yyyy-MM-dd"));
            updateQuery.bindValue(":id", id);
            updateQuery.exec();

            logTransaction(clientTable->item(id-1, 0)->text(), "Interest", balanceInterest - loanInterest);
            logTransaction(clientTable->item(id-1, 0)->text(), "Loan Interest Charged", -loanInterest);
            logTransaction("System", "Interest Applied", balanceInterest - loanInterest);
            logTransaction("System", "Loan Interest Charged", -loanInterest);
        }
                loadClients();
        } else {
        //  qDebug() << "Yes was *not* clicked";
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
    QLineEdit *amountIntrest;
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
