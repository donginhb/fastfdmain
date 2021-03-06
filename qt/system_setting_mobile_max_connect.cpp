#include "system_setting_mobile_max_connect.h"
#include "ui_system_setting_mobile_max_connect.h"
#include "lds.h"
#include "n_func.h"
#include "lds_messagebox.h"
#include <QtDebug>
#include <QClipboard>
#include "getfilepath.h"

system_setting_mobile_max_connect::system_setting_mobile_max_connect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::system_setting_mobile_max_connect)
{
    ui->setupUi(this);
    ui->lineEdit_hardware_id->setText(lds::getHardWareID_unique());
#ifdef QT_DEBUG
    ui->lineEdit_hardware_id->setText("80612144757403C");

#endif
    ui->lineEdit_mobile_connect_max_value->setValue(n_func::f_get_sysparm("mobile_connect_max_value", "1").toInt());
    connect(ui->pushButton_cancel,SIGNAL(clicked()),this,SLOT(tocancel()));
    connect(ui->pushButton_ok,SIGNAL(clicked()),this,SLOT(took()));
    connect(ui->toolButton_copy, SIGNAL(clicked()),this,SLOT(tocopy()));

}

system_setting_mobile_max_connect::~system_setting_mobile_max_connect()
{
    delete ui;
}

QString system_setting_mobile_max_connect::get_id_value(const QString &hardid, const QString &value)
{
    return lds::of_str_encrypt(QString("%1(%2)")
                                    .arg(hardid)
                                    .arg(value));
}

void system_setting_mobile_max_connect::took()
{
    // 124578(1)
    QString id_value = lds::of_str_decrypt(QByteArray::fromHex(ui->lineEdit_auth_code->text().toLocal8Bit()));//解密的结果会忽略所有空格
    QString value;
    QString id;
    QString errstring;
    errstring = tr("格式错误") + QString("%1").arg(__LINE__);
    if(id_value.endsWith(")") && id_value.contains("(")){
        id_value.chop(1);
        for(int max = id_value.count() - 1; max >= 0; max --){
            if(id_value.at(max) == QChar('(')){
                id_value.chop(1);
                break;
            }
            value.push_front(id_value.at(max));
            id_value.chop(1);
        }
    }
    id = id_value;
    bool ok;
    value.toInt(&ok);
    errstring = tr("机器号不匹配") + QString("%1").arg(__LINE__);
    qDebug() << "id:" << id   << (id == ui->lineEdit_hardware_id->text());

    if(id == ui->lineEdit_hardware_id->text().replace(" ","")){//解密的结果会忽略所有空格
        errstring = tr("连接数不匹配") + QString("%1").arg(__LINE__);
        if(value.toInt() == ui->lineEdit_mobile_connect_max_value->value()){
            lds_messagebox::information(this, MESSAGE_TITLE_VOID, tr("操作成功") + "\n" +tr("移动端最大连接数")+":"+value);
            n_func::f_set_sysparm("mobile_connect_max_value", ui->lineEdit_mobile_connect_max_value->value(), tr("移动端最大连接数"));
            this->accept();
            return;
        }
    }
    lds_messagebox::warning(this, MESSAGE_TITLE_VOID, errstring);
}

void system_setting_mobile_max_connect::tocancel()
{
    this->reject();
}

void system_setting_mobile_max_connect::on_toolButton_clicked()
{
    //获取路径
    QString filepath = getFilePath::getOpenFileName(this, tr("导入文件"));
    if(!filepath.isEmpty()){
        QFile file(filepath);
        if(file.open(QFile::ReadOnly)){
            ui->lineEdit_auth_code->setText(file.readLine());
            file.close();
            return;
        }
        lds_messagebox::warning(this, MESSAGE_TITLE_VOID, file.errorString());
        return;
    }
}

void system_setting_mobile_max_connect::tocopy()
{
    QClipboard *board = QApplication::clipboard();
    board->setText(ui->lineEdit_hardware_id->text());
}
