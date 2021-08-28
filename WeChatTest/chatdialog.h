#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include"customwidget.h"
#include"QVBoxLayout"
#include"fileitem.h"
#include<QFont>
#include"bqForm.h"

namespace Ui {
class ChatDialog;
}

class ChatDialog : public /*QDialog*/CustomMoveDialog
{
    Q_OBJECT
signals:
    void SIG_SendChatMsg( int id , QString content);
    void SIG_SendFile( int id ,QString path);
public:
    explicit ChatDialog(QWidget *parent = 0);
    ~ChatDialog();

private:
    Ui::ChatDialog *ui;


public:
    int m_id;
    QString m_name;
    QVBoxLayout * m_fileLayout;
    QFont m_font;
    bqForm *m_bq;
public slots:
    void on_pb_send_clicked();
    void slot_recvChatMsg( QString content );
    void slot_showOffline();
    void setInfo( int id , QString name);

    void on_pb_min_clicked();
    void on_pb_max_clicked();
    void on_pb_close_clicked();
    void on_pb_tool2_clicked();
    void on_pb_hide_clicked();
    void slot_addFileItem( FileItem* item );
    void slot_InsertBQToEdit(qint32 id);

    void on_pb_tool3_clicked();
    void on_pb_tool1_clicked();
};

#endif // CHATDIALOG_H
