 
void transferFunds(int senderId, int receiverId, double amount) {
    if (amount <= 0) {
        QMessageBox::warning(nullptr, "Invalid Transfer", "Amount must be greater than zero.");
        return;
    }

    QSqlQuery senderQuery;
    senderQuery.prepare("SELECT balance, name FROM accounts WHERE id = :senderId");
    senderQuery.bindValue(":senderId", senderId);
    senderQuery.exec();

    if (!senderQuery.next()) {
        QMessageBox::warning(nullptr, "Invalid Sender", "Sender account not found.");
        return;
    }

    double senderBalance = senderQuery.value(0).toDouble();
    QString senderName = senderQuery.value(1).toString();

    if (senderBalance < amount) {
        QMessageBox::warning(nullptr, "Insufficient Funds", "Sender does not have enough balance.");
        return;
    }

    QSqlQuery receiverQuery;
    receiverQuery.prepare("SELECT name FROM accounts WHERE id = :receiverId");
    receiverQuery.bindValue(":receiverId", receiverId);
    receiverQuery.exec();

    if (!receiverQuery.next()) {
        QMessageBox::warning(nullptr, "Invalid Receiver", "Receiver account not found.");
        return;
    }

    QString receiverName = receiverQuery.value(0).toString();

    QSqlQuery transaction;
    transaction.prepare("UPDATE accounts SET balance = balance - :amount WHERE id = :senderId");
    transaction.bindValue(":amount", amount);
    transaction.bindValue(":senderId", senderId);
    transaction.exec();

    transaction.prepare("UPDATE accounts SET balance = balance + :amount WHERE id = :receiverId");
    transaction.bindValue(":amount", amount);
    transaction.bindValue(":receiverId", receiverId);
    transaction.exec();

    logTransaction(senderName, "Transfer Sent", -amount);
    logTransaction(receiverName, "Transfer Received", amount);

    QMessageBox::information(nullptr, "Transfer Successful", "Funds successfully transferred from " + senderName + " to " + receiverName);
}

