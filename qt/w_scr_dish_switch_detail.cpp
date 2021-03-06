#include "w_scr_dish_switch_detail.h"
#include "ui_w_scr_dish_switch_detail.h"
#include <QSqlRecord>
#include "n_func.h"
#include "backheader.h"
#include "lds_messagebox.h"
#include <QSqlError>
#include "lds_dialog_input.h"
#include "lds.h"
#include <QSqlRecord>
#include "w_m_member.h"
#include <QSqlQueryModel>
#include "lds_dialog_input_double.h"
#include "ui_lds_dialog_input_dialog_double.h"
#include "lds_tableview.h"
#include <QtDebug>
#include <QWidgetAction>
#include "lds_menu.h"

w_scr_dish_switch_detail::w_scr_dish_switch_detail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::w_scr_dish_switch_detail)
{
    ui->setupUi(this);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    ui->checkBox->setCheckedByIni("w_scr_dish_switch_detail/isprint", false);
    ui->checkBox_print_detail->setCheckedByIni("w_scr_dish_switch_detail/isprintdetail", false);
    ui->checkBox_cash_fee_keep_to_accept->setCheckedByIni("w_scr_dish_switch_detail/cash_fee_keep_to_accept", true);
    ui->checkBox_print_sale_detail->setCheckedByIni("w_scr_dish_switch_detail/print_sale_detail", true);

    int_id = -1;
    ui->pushButton_printcount->setNumber(lds::sysconf->value("w_scr_dish_switch_detail/printcount", 1).toDouble());
    ui->pushButton_cash_register->setNumber(get_cey_sys_operator(public_sql::gs_operid, "num_shift_cash_register").toDouble());

    //
    ui->tableWidget->setFocusPolicy(Qt::NoFocus);
    ui->horizontalLayout_3->removeWidget(ui->frame_set);
    ui->frame_set->setMinimumWidth(250);
    lds_menu *menu = new lds_menu(this);
    QWidgetAction *widgetaction = new QWidgetAction(ui->pushButton_set);
    widgetaction->setDefaultWidget(ui->frame_set);
    menu->addAction(widgetaction);
    ui->pushButton_set->setMenu(menu);
    //
    connect(ui->pushButton_printcount, SIGNAL(clicked()),this,SLOT(to_set_print_count()));
    connect(ui->pushButton_cash_register, SIGNAL(clicked()),this,SLOT(to_set_cash_register()));
    connect(ui->pushButton_print, SIGNAL(clicked()),this,SLOT(toprint()));
    connect(ui->pushButton_finish, SIGNAL(clicked()),this,SLOT(tofinish()));
    connect(ui->pushButton_switch, SIGNAL(clicked()),this,SLOT(toswitch()));
    connect(ui->pushButton_exit, SIGNAL(clicked()),this,SLOT(toexit()));
    connect(ui->checkBox_print_detail,SIGNAL(toggled(bool)),this,SLOT(tosaveprintdetail(bool)));
    connect(ui->checkBox_cash_fee_keep_to_accept,SIGNAL(toggled(bool)),this,SLOT(tosave_cash_fee_keep_to_accept(bool)));
    connect(ui->pushButton_fetch, SIGNAL(clicked()), this, SLOT(tofetch()));
    connect(ui->pushButton_fetch_record, SIGNAL(clicked()), this, SLOT(tofetchrecord()));
    connect(ui->lineEdit_vch_memo, SIGNAL(outfocus()), this, SLOT(update_shift_memo()));
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(update_temp_handover()));
    connect(ui->checkBox_print_sale_detail, SIGNAL(signal_switch(QVariant)), this, SLOT(toInitCommitData()));

    ui->pushButton_set->setFocusPolicy(Qt::NoFocus);

    setWindowTitle(tr("营业交班"));
}

w_scr_dish_switch_detail::~w_scr_dish_switch_detail()
{
    delete ui;
}

bool w_scr_dish_switch_detail::rightChecked(QWidget *parent)
{
    return true;
}

void w_scr_dish_switch_detail::initData()
{
    ui->pushButton_print->hide();
    ui->pushButton_finish->hide();
    lds_query query;
    //1
    date1 = n_func::f_get_sysdatetime();

    query.execSelect(QString("select int_id, dt_begin, dt_end from cey_u_handover_master where vch_operid ='%1'  order by int_id desc limit 0, 1")
                     .arg(public_sql::gs_operid));
    query.next();
    date0 = query.recordValue("dt_end").toDateTime();
    if(date0.isNull()){
        query.execSelect(QString("select Min( dt_operdate)from cey_u_checkout_master where vch_operid ='%1' ")
                         .arg(public_sql::gs_operid));
        query.next();
        date0 = query.recordValue(0).toDateTime();
    }
    if(date0.isNull()){
        date0 = date1;
    }

    //2
    ui->tableWidget->setSpan(0,1,1,2);
    ui->tableWidget->setSpan(2,1,1,3);
    ui->tableWidget->setSpan(3,1,1,3);
    ui->tableWidget->setSpan(4,1,1,3);
    ui->tableWidget->setRowHeight(0, 25);
    ui->tableWidget->setColumnWidth(2, 60);//x12
    ui->tableWidget->setColumnWidth(3, 100);//12.09
    //2.1
    //交班人
    if(public_sql::gs_operid == "0000")  {
        ui->tableWidget->item(1,1)->setText("system(0000)");
    }
    else
    {
        ui->tableWidget->item(1,1)->setText(public_sql::gs_operid);
    }
    //开始时间
    ui->tableWidget->item(2,1)->setText(date0.toString(yyyyMMddhhmmss));
    //交班时间
    ui->tableWidget->item(3,1)->setText(date1.toString(yyyyMMddhhmmss));
    //备注
    ui->tableWidget->setItem(4,1, new QTableWidgetItem);

    toInitCommitData();
    return;
}

bool w_scr_dish_switch_detail::commitData(QString &errstring, int &int_id, bool isForPrevious)
{

    double ldec_numcost = 0;
    double ldec_lowcost = 0;
    double ldec_service = 0;
    double ldec_tax = 0;
    double ldec_sumdiscount = 0;
    double ldec_sumpresent = 0;
    QPair<QString, double> ldec_paymode_xx;ldec_paymode_xx.second = 0;
    QPair<QString, double> ldec_paymode_yy;ldec_paymode_yy.second = 0;
    QPair<QString, double> ldec_paymode_zz;ldec_paymode_zz.second = 0;
    QPair<QString, double> ldec_paymode_11;ldec_paymode_11.second = 0;
    QList< QPair<QString, double> > ldec_consum_pay_list;
    QList< QPair<QString, double> > ldec_paymode_otherlist;
    double ldec_paymode_other_00 = 0;
    QList< QPair<QString, double> > ldec_m_num_realamount_pay_list;
    double ldec_m_num_deposit = 0;//本地充值
    double ldec_m_num_realamount = 0;//本地实收
    double ldec_m_num_realamount_00 = 0;//本地实收现金
    double ldec_m_num_back = 0;//本地退卡
    double ldec_m_num_fetch = 0;//本地取款
    double ldec_num_sumback=0;//退菜数量
    double ldec_num_sumbackcost=0;//退菜金额
    double taishu = 0;
    w_sys_manage_cloudsync::member_r_f_b_info m_rfb;
    lds_query query;
    int sqlindex=1;
    bool exec_flag = true;

    lds_messagebox_loading_show loading(this, MESSAGE_TITLE_VOID, "LOADING");
    loading.show_delay();
    //1
    QString vch_operID = public_sql::gs_operid;
    QString vch_accept = new_operid;
    QString vch_memo = handover_tmp_vch_memo();
    lds_model_sqltablemodel cache;

    query.execSelect(QString("select max(int_id)  from cey_u_handover_master ")
                     //.arg(public_sql::gs_operid)
                     );
    query.next();
    int_id = query.recordValue(0).toInt();
    int_id++;


    if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_master",
                                                                                                                          qrtVariantPairList()
                                                                                                                          << qrtVariantPair("int_id", int_id)
                                                                                                                          << qrtVariantPair("vch_operID", vch_operID)
                                                                                                                          << qrtVariantPair("dt_begin", date0.toString(yyyyMMddhhmmss))
                                                                                                                          << qrtVariantPair("dt_end", date1.toString(yyyyMMddhhmmss))
                                                                                                                          << qrtVariantPair("vch_accept", vch_accept)

                                                                                                                          << qrtVariantPair("vch_memo", vch_memo) );
    //!------------------begin
    {
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("SELECT IfNull( sum( num_cost ) , 0 ) as sumcost , IfNull( sum(num_discount ) , 0 ) sumdiscount, IfNull(sum( num_present ) , 0 ) as sumpresent, IfNull( sum( num_service ) , 0 ) as sumservice, IfNull( sum( num_lowcost ) , 0 ) as sumlowcost,"
                                                                                                                                      " IfNull( sum( num_blotout ) , 0 ) as sumblotout,"
                                                                                                                                      " IfNull( sum( num_free ) , 0 ) as sumfree,"
                                                                                                                                      " IfNull( sum( num_rate ) , 0 ) as sumtax"
                                                                                                                                      " from cey_u_checkout_master"
                                                                                                                                      " where ch_state ='Y' and  ( vch_operID ='%1' ) and"
                                                                                                                                      " ( dt_operdate >= '%2' and dt_operdate <= '%3' )")
                                                                                                                              .arg(public_sql::gs_operid)
                                                                                                                              .arg(date0.toString(yyyyMMddhhmmss))
                                                                                                                              .arg(date1.toString(yyyyMMddhhmmss)));
        query.next();
        qDebug() << __LINE__<< query.lastQuery();

        ldec_numcost = query.recordValue("sumcost").toDouble();
        ldec_service = query.recordValue("sumservice").toDouble();
        ldec_tax = query.recordValue("sumtax").toDouble();
        ldec_lowcost = query.recordValue("sumlowcost").toDouble();
        ldec_sumdiscount = query.recordValue("sumdiscount").toDouble();
        ldec_sumpresent = query.recordValue("sumpresent").toDouble();
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("select  sum(num_back) as sumback,"
                                                                                                                                      " sum(num_price*int_discount*num_back/100.0)"
                                                                                                                                      " from cey_u_orderdish a where"
                                                                                                                                      " a.vch_operID ='%1'  and "
                                                                                                                                      " a.dt_operdate >= '%2' and a.dt_operdate <= '%3' and a.num_back > 0 and ch_presentflag <> 'Y'"
                                                                                                                                      " group by ch_payno")
                                                                                                                              .arg(public_sql::gs_operid)
                                                                                                                              .arg(date0.toString(yyyyMMddhhmmss))
                                                                                                                              .arg(date1.toString(yyyyMMddhhmmss)));
        
        while(query.next()){
            ldec_num_sumback += query.recordValue(0).toDouble();
            ldec_num_sumbackcost += qAbs(query.recordValue(1).toDouble());
        }
        //=============
        ldec_paymode_other_00 = 0;
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("select (select aa.vch_paymodename from cey_bt_paymode aa where aa.ch_paymodeno=a.ch_paymodeno) as paymodename ,a.ch_paymodeno,  count(a.ch_paymodeno) as count, sum(a.num_realamount) as realamount, sum(a.num_face) as face"
                                                                                                                                      " from cey_u_checkout_detail a , cey_u_checkout_master b where ( a.ch_payno =b.ch_payno ) and ( b.ch_state ='Y' ) and ( b.vch_operID ='%1' ) and ( b.dt_operdate >= '%2' and b.dt_operdate <= '%3' )and a.ch_paymodeno <> ''"
                                                                                                                                      " Group by a.ch_paymodeno Order by a.ch_paymodeno")
                                                                                                                              .arg(public_sql::gs_operid)
                                                                                                                              .arg(date0.toString(yyyyMMddhhmmss))
                                                                                                                              .arg(date1.toString(yyyyMMddhhmmss)));

        while(query.next()){
            if(query.recordValue("realamount").toDouble() != 0){
                if(query.recordValue("ch_paymodeno").toString() == "XX"){
                    ldec_paymode_xx.first = query.recordValue("paymodename").toString();
                    ldec_paymode_xx.second = query.recordValue("realamount").toDouble();
                } else if(query.recordValue("ch_paymodeno").toString() == "YY"){//抹零
                    ldec_paymode_yy.first = query.recordValue("paymodename").toString();
                    ldec_paymode_yy.second = query.recordValue("realamount").toDouble();
                } else if(query.recordValue("ch_paymodeno").toString() == "ZZ"){//优惠
                    ldec_paymode_zz.first = query.recordValue("paymodename").toString();
                    ldec_paymode_zz.second = query.recordValue("realamount").toDouble();
                }  else if(query.recordValue("ch_paymodeno").toString() == "11"){//挂账
                    ldec_paymode_11.first = query.recordValue("paymodename").toString();
                    ldec_paymode_11.second = query.recordValue("realamount").toDouble();
                }  else {
                    if(query.recordValue("ch_paymodeno").toString() == "00"){//现金,保证在最后一个
                        ldec_paymode_otherlist.append(QPair<QString, double>(query.recordValue("paymodename").toString(),
                                                                             query.recordValue("realamount").toDouble()));
                        ldec_paymode_other_00 = query.recordValue("realamount").toDouble();
                    } else {
                        ldec_paymode_otherlist.prepend(QPair<QString, double>(query.recordValue("paymodename").toString(),
                                                                              query.recordValue("realamount").toDouble()));
                    }
                }
            }
        }

        qDebug() << "ldec_paymode_otherlist :" << ldec_paymode_otherlist << query.lastQuery();
        updateKeyFix4hanzi(ldec_paymode_otherlist);
        //=======
        //充值金额
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("select ifnull(sum(num_deposit),0) from t_m_deposit  "
                                                                                                                                      " where (vch_operID ='%1' ) and ch_deposit_mode='1' and "
                                                                                                                                      " ( dt_operdate >= '%2' and dt_operdate <= '%3' )")
                                                                                                                              .arg(public_sql::gs_operid)
                                                                                                                              .arg(date0.toString(yyyyMMddhhmmss))
                                                                                                                              .arg(date1.toString(yyyyMMddhhmmss)));
        query.next();
        ldec_m_num_deposit = query.recordValue(0).toDouble();

        //充值实收明细
        //常规充值
        ldec_m_num_realamount = 0;
        query.execSelect(QString("SELECT   ch_pay_mode, ifnull(sum(num_realamount), 0) from t_m_deposit where vch_operID = '%1' and ch_deposit_mode = '1' and dt_operdate >= '%2'  and dt_operdate <= '%3' group by ch_pay_mode ")
                         .arg(public_sql::gs_operid)
                         .arg(date0.toString(yyyyMMddhhmmss))
                         .arg(date1.toString(yyyyMMddhhmmss)));
        while(query.next()) {
            QList<QStandardItem*> items = w_m_member::get_RECHARGE_MODEL_PAYMODE_RECHARGE().findItems(query.recordValue(0).toString());
            if(items.count() > 0) {
                //保证现金在最后面
                if(query.recordValue(0).toString() == "1") {
                    ldec_m_num_realamount_pay_list.append(QPair<QString, double>(w_m_member::get_RECHARGE_MODEL_PAYMODE_RECHARGE().item(items[0]->row(), 1)->text(),
                                                          query.recordValue(1).toDouble()));
                    //仅现金充值
                    ldec_m_num_realamount_00 = query.recordValue(1).toDouble();
                } else {
                    ldec_m_num_realamount_pay_list.prepend(QPair<QString, double>(w_m_member::get_RECHARGE_MODEL_PAYMODE_RECHARGE().item(items[0]->row(), 1)->text(),
                                                           query.recordValue(1).toDouble()));
                }
                //所有充值
                ldec_m_num_realamount += query.recordValue(1).toDouble();
            }
        }
        updateKeyFix4hanzi(ldec_m_num_realamount_pay_list);
        //会员退卡5 取款8
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("select ifnull(sum(num_deposit),0) from t_m_deposit  "
                                                                                                                                      " where (vch_operID ='%1' ) and (ch_deposit_mode='5') and"
                                                                                                                                      " ( dt_operdate >= '%2' and dt_operdate <= '%3' )")
                                                                                                                              .arg(public_sql::gs_operid)
                                                                                                                              .arg(date0.toString(yyyyMMddhhmmss))
                                                                                                                              .arg(date1.toString(yyyyMMddhhmmss)));
        query.next();
        ldec_m_num_back = query.recordValue(0).toDouble();
        //取款8
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("select ifnull(sum(num_deposit),0) from t_m_deposit  "
                                                                                                                                      " where (vch_operID ='%1' ) and (ch_deposit_mode='8') and"
                                                                                                                                      " ( dt_operdate >= '%2' and dt_operdate <= '%3' )")
                                                                                                                              .arg(public_sql::gs_operid)
                                                                                                                              .arg(date0.toString(yyyyMMddhhmmss))
                                                                                                                              .arg(date1.toString(yyyyMMddhhmmss)));
        query.next();
        ldec_m_num_fetch = query.recordValue(0).toDouble();

        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execSelect(QString("select count(a.ch_tableno) from cey_u_table a , cey_u_checkout_master b where a.ch_payno =b.ch_payno and b.ch_state ='Y' and(b.vch_operID ='%1')and(b.dt_operdate >= '%2' and b.dt_operdate <='%3' )")
                                                                                                                              .arg(public_sql::gs_operid).arg(date0.toString(yyyyMMddhhmmss)).arg(date1.toString(yyyyMMddhhmmss)));
        query.next();
        taishu = query.recordValue(0).toDouble();

        //云会员卡信息
        //        QString rechargeAmount;//充值金额
        //        QString realRechargeAmount;//充值实收
        //        QString withdrawalAmount;//取款金额
        //        QString backCardAmount;//退卡金额
        if(lds::sysconf->value("w_sys_manage_cloudsync_with_time/cloud_yun_member", false).toBool()){
            bool ok;
            m_rfb=
                    w_sys_manage_cloudsync::get_yun_member_r_f_b(this, public_sql::gs_operid,date0.toString(yyyyMMddhhmmss),date1.toString(yyyyMMddhhmmss),ok);
        }
    }
    //!------------------end
    //结账台数
    if(taishu!=0){
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("结账台数"))<< qrtVariantPair("num_amount", taishu)<< qrtVariantPair("vch_item2", "")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
    }
    //退菜数量
    if(ldec_num_sumback != 0){
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("退菜数量"))<< qrtVariantPair("num_amount", ldec_num_sumback)<< qrtVariantPair("vch_item2", "")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
    }
    //退菜金额
    if(ldec_num_sumbackcost != 0) {
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("退菜金额"))<< qrtVariantPair("num_amount", ldec_num_sumbackcost)<< qrtVariantPair("vch_item2", "")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
    }
    //消费金额
    if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                          << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", updateKeyFix4hanzi(tr("消费金额")))<< qrtVariantPair("num_amount", ldec_numcost)<< qrtVariantPair("vch_item2", "P")
                                                                                                                          << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
    //服务费
    if(ldec_service != 0){
        ldec_consum_pay_list << QPair<QString, double>(tr("服务费"), ldec_service);
        qDebug() << __LINE__ << ldec_consum_pay_list.last().first;
    }
    //税率
    if(ldec_tax != 0){
        ldec_consum_pay_list << QPair<QString, double>(tr("税收"), ldec_tax);
        qDebug() << __LINE__ << ldec_consum_pay_list.last().first;
    }
    //低消
    if(ldec_lowcost != 0){
        ldec_consum_pay_list << QPair<QString, double>(tr("低消差"), ldec_lowcost);
        qDebug() << __LINE__ << ldec_consum_pay_list.last().first << ldec_consum_pay_list.last().second;
    }
    //折扣金额
    if(ldec_sumdiscount != 0){
        ldec_consum_pay_list << QPair<QString, double>(tr("折扣金额"), ldec_sumdiscount);
    }
    //赠送金额
    if(ldec_sumpresent != 0){
        ldec_consum_pay_list << QPair<QString, double>(tr("赠送金额"), ldec_sumpresent);
    }
    //        xx
    if(ldec_paymode_xx.second != 0){
        ldec_consum_pay_list << QPair<QString, double>(ldec_paymode_xx.first, ldec_paymode_xx.second);
    }
    //        yy自动抹零
    if(ldec_paymode_yy.second != 0){
        ldec_consum_pay_list << QPair<QString, double>(ldec_paymode_yy.first, ldec_paymode_yy.second);
    }
    //        zz 优惠
    if(ldec_paymode_zz.second != 0){
        ldec_consum_pay_list << QPair<QString, double>(ldec_paymode_zz.first, ldec_paymode_zz.second);
    }
    //        11 会员挂账
    if(ldec_paymode_11.second != 0){
        ldec_consum_pay_list << QPair<QString, double>(ldec_paymode_11.first, ldec_paymode_11.second);
    }
    updateKeyFix4hanzi(ldec_consum_pay_list);
    for(int k = 0; k < ldec_consum_pay_list.count(); k ++) {
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", ldec_consum_pay_list[k].first)<< qrtVariantPair("num_amount", ldec_consum_pay_list[k].second)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
    }


    //实收金额
    double realamountall=0.0;
    for(int index = 0, indexcount = ldec_paymode_otherlist.count();index<indexcount;index++){
        realamountall+=ldec_paymode_otherlist[index].second;
    }
    if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                          << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("实收金额"))<< qrtVariantPair("num_amount", realamountall)<< qrtVariantPair("vch_item2", "P")
                                                                                                                          << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));

    //其他付款
    for(int index = 0, indexcount = ldec_paymode_otherlist.count();index<indexcount;index++){
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", ldec_paymode_otherlist[index].first)<< qrtVariantPair("num_amount", ldec_paymode_otherlist[index].second)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
    }
    //会员卡
    if(ldec_m_num_deposit != 0
            || ldec_m_num_realamount_pay_list.count()!=0
            || ldec_m_num_back != 0
            || ldec_m_num_fetch != 0

            || m_rfb.backCardAmount.toDouble() != 0
            || m_rfb.realRechargeAmount.toDouble() != 0
            || m_rfb.rechargeAmount.toDouble() != 0
            || m_rfb.withdrawalAmount.toDouble() != 0
            ) {//
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("会员卡统计"))<< qrtVariantPair("vch_item2", "PN")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        //充值金额
        if(ldec_m_num_deposit!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("充值金额"))<< qrtVariantPair("num_amount", ldec_m_num_deposit)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
        //充值实收
        if(ldec_m_num_realamount_pay_list.count()!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("充值实收"))<< qrtVariantPair("num_amount", ldec_m_num_realamount)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));

            for(int k = 0; k < ldec_m_num_realamount_pay_list.count(); k++) {
                if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                      << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", ldec_m_num_realamount_pay_list[k].first)<< qrtVariantPair("num_amount", ldec_m_num_realamount_pay_list[k].second)<< qrtVariantPair("vch_item2", "YY")
                                                                                                                                      << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
            }
        }
        //会员退卡取款
        if(ldec_m_num_back!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("会员退卡取款"))<< qrtVariantPair("num_amount", ldec_m_num_back)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
        //会员取款
        if(ldec_m_num_fetch!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("会员取款"))<< qrtVariantPair("num_amount", ldec_m_num_fetch)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
        //充值金额(云)
        if(m_rfb.rechargeAmount.toDouble()!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("充值金额(云)"))<< qrtVariantPair("num_amount", m_rfb.rechargeAmount)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
        //充值实收(云)
        if(m_rfb.realRechargeAmount.toDouble()!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("充值实收(云)"))<< qrtVariantPair("num_amount", m_rfb.realRechargeAmount)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
        //会员取款(云)
        if(m_rfb.withdrawalAmount.toDouble()!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("会员取款(云)"))<< qrtVariantPair("num_amount", m_rfb.withdrawalAmount)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
        //会员取款(云)
        if(m_rfb.backCardAmount.toDouble()!=0){
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("会员退卡(云)"))<< qrtVariantPair("num_amount", m_rfb.backCardAmount)<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }
    }

    if(false == isForPrevious) {
        double cash_register_fetch = 0;
        //备用金
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("备用金"))<< qrtVariantPair("num_amount", ui->pushButton_cash_register->number())<< qrtVariantPair("vch_item2", "P")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));

        //取款
        {
            lds_query qry;
            qry.execSelect(QString(" select int_id from cey_u_handover_master where int_id < 0 and vch_operID = '%1' ")
                           .arg(public_sql::gs_operid));
            if(qry.next()) {
                int qry_int_id;
                qry_int_id = qry.recordValue("int_id").toInt();

                qry.execSelect(QString("select int_id, int_row, vch_item, num_amount from cey_u_handover_total  where int_id = '%1'  ")
                               .arg(qry_int_id));
                while(qry.next()) {
                    if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                          << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", qry.recordValue("vch_item"))<< qrtVariantPair("num_amount", qry.recordValue("num_amount"))<< qrtVariantPair("vch_item2", "P")
                                                                                                                                          << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
                    cash_register_fetch += qry.recordValue("num_amount").toDouble();
                }
                if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execDelete("cey_u_handover_total", qrtEqual("int_id", qry_int_id));
                if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execDelete("cey_u_handover_master", qrtEqual("int_id", qry_int_id));
            }
        }
        //钱箱现金
        double moneytotal = ui->pushButton_cash_register->number();
        moneytotal += ldec_paymode_other_00;//现金
        moneytotal += ldec_m_num_realamount_00;//充值实收现金
        moneytotal -= ldec_m_num_back;//退卡
        moneytotal -= ldec_m_num_fetch;//取款

        moneytotal += m_rfb.realRechargeAmount.toDouble();//（云）充值实收
        moneytotal -= m_rfb.withdrawalAmount.toDouble();//（云）退卡
        moneytotal -= m_rfb.backCardAmount.toDouble();//（云）取款

        moneytotal -= cash_register_fetch;//钱箱取款
        if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                              << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("钱箱现金"))<< qrtVariantPair("num_amount", moneytotal)<< qrtVariantPair("vch_item2", "P")
                                                                                                                              << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));

        query.execSelect(QString("select vch_operID, vch_accept from cey_u_handover_master where int_id = %1;")
                         .arg(int_id));
        query.next();
        QString vch_operID = query.recordValue("vch_operID").toString();
        QString vch_accept = query.recordValue("vch_accept").toString();
        if(vch_accept.length() > 0) {
            double vch_accept_num_shift_cash_register = 0;
            if(lds::sysconf->value("w_scr_dish_switch_detail/cash_fee_keep_to_accept", true).toBool()) {
                vch_accept_num_shift_cash_register =ui->pushButton_cash_register->number();
            }
            set_cey_sys_operator(vch_accept, "ch_shift_config_flag", "N");
            set_cey_sys_operator(vch_accept, "num_shift_cash_register", vch_accept_num_shift_cash_register);
        }
        set_cey_sys_operator(vch_operID, "ch_shift_config_flag", "N");
        set_cey_sys_operator(vch_operID, "num_shift_cash_register", 0);
    }

    //销售明细
    if(lds::sysconf->value("w_scr_dish_switch_detail/print_sale_detail", false).toBool()) {
        QMap<QString, QVariantList>  dish_numTotal_map;
        QMap<QString, double> ch_dish_typeno_total_map;
        QMap<QString, double> ch_sub_typeno_total_map;
        query.execSelect(QString(
                             "                    select "
                             "                        a.ch_dishno,"
                             "                        sum(a.num_num - a.num_back) as num_dish,"
                             "                        sum(if(a.ch_presentflag = 'Y',"
                             "                            0,"
                             "                            (a.num_num - a.num_back) * a.num_price * a.int_discount * 0.01) + a.num_price_add) as num_total"
                             "                    from"
                             "                        cey_u_orderdish a,"
                             "                        cey_u_checkout_master b"
                             "                    where"
                             "                        a.ch_payno = b.ch_payno"
                             "                            and b.ch_state = 'Y'"
                             "                            and b.vch_operid = '%1' "
                             "                            and b.dt_operdate >= '%2' "
                             "                            and b.dt_operdate <= '%3' "
                             "                    group by a.ch_dishno;"
                             )
                         .arg(public_sql::gs_operid)
                         .arg(date0.toString(yyyyMMddhhmmss))
                         .arg(date1.toString(yyyyMMddhhmmss))
                         );
        qDebug() << query.lastQuery();
        while(query.next()) {
            QString dishno = query.recordValue("ch_dishno").toString();
            QString key;
            QString type = cache.tablename_kvmap_value("cey_bt_dish", dishno, "ch_dishno", "ch_dish_typeno").toString();
            QString subtype = cache.tablename_kvmap_value("cey_bt_dish", dishno, "ch_dishno", "ch_sub_typeno").toString();
            QString dish = dishno;

            key += type + ",";
            key += subtype + ",";
            key += dish;
            dish_numTotal_map.insert(key,  QVariantList() << query.recordValue("num_dish") << query.recordValue("num_total"));
            ch_dish_typeno_total_map.insert(type, ch_dish_typeno_total_map.value(type, 0) + query.recordValue("num_total").toDouble());
            ch_sub_typeno_total_map.insert(subtype, ch_sub_typeno_total_map.value(subtype, 0) + query.recordValue("num_total").toDouble());
        }

        if(dish_numTotal_map.count() > 0) {
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", tr("类别汇总")) << qrtVariantPair("num_amount", 0)<< qrtVariantPair("vch_item2", "L")
                                                                                                                                  << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
        }

        QMapIterator<QString, QVariantList>  mapitr(dish_numTotal_map);
        while(mapitr.hasNext()) {
            mapitr.next();
            QStringList keys = mapitr.key().split(",");
            QString type = keys.value(0);//type
            QString subtype = keys.value(1);//subtype
            QString dish = keys.value(2);//dish
            double dish_num = mapitr.value()[0].toDouble();
            double dish_total = mapitr.value()[1].toDouble();

            if(ch_dish_typeno_total_map.contains(type)) {//type
                if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                      << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", cache.tablename_kvmap_value("cey_bt_dish_type", type, "ch_dish_typeno", "vch_dish_typename")) << qrtVariantPair("num_amount", ch_dish_typeno_total_map.value(type))<< qrtVariantPair("vch_item2", "P")
                                                                                                                                      << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
                ch_dish_typeno_total_map.remove(type);
            }
            if(ch_sub_typeno_total_map.contains(subtype)) {//subtype
                if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                      << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", cache.tablename_kvmap_value("cey_bt_dish_subtype", subtype, "ch_sub_typeno", "vch_sub_typename")) << qrtVariantPair("num_amount", ch_sub_typeno_total_map.value(subtype))<< qrtVariantPair("vch_item2", "Y")
                                                                                                                                      << qrtVariantPair("num_amount2", 0)<< qrtVariantPair("int_height", "80"));
                ch_sub_typeno_total_map.remove(subtype);
            }
            if(!exec_flag) {errstring = QString::number(__LINE__)+query.recordError(); return false;}exec_flag = query.execInsert("cey_u_handover_total",qrtVariantPairList()
                                                                                                                                  << qrtVariantPair("int_id", int_id)<< qrtVariantPair("int_row", sqlindex++)<< qrtVariantPair("vch_item", cache.tablename_kvmap_value("cey_bt_dish", dish, "ch_dishno", "vch_dishname").toString()) << qrtVariantPair("num_amount", dish_total)<< qrtVariantPair("vch_item2", "YYX")
                                                                                                                                  << qrtVariantPair("num_amount2", dish_num)<< qrtVariantPair("int_height", "80"));
        }
    }

    return true;
}

void w_scr_dish_switch_detail::updateKeyFix4hanzi(QList<QPair<QString, double> > &ldec_paymode_otherlist)
{
    for(int index = 0, indexcount = ldec_paymode_otherlist.count(); index < indexcount; index++){
        ldec_paymode_otherlist[index].first = updateKeyFix4hanzi(ldec_paymode_otherlist[index].first);
    }
}

QString w_scr_dish_switch_detail::updateKeyFix4hanzi(const QString &payname)
{
    QString name = payname;
    if(payname.count() == 2) {
        name = QString() + payname[0]+"  "+"  "+payname[1];
    } else if(payname.count() == 3) {
        name = QString() + payname[0]+" "+payname[1]+" "+payname[2];
    }
    return name;
}

QVariant w_scr_dish_switch_detail::get_cey_sys_operator(const QString &operid, const QString &valuename)
{
    QVariant var;
    lds_query query;
    if(operid  == "0000") {
        var = n_func::f_get_sysparm("system_"+valuename, "1");
    } else {;
        query.execSelect(QString("select "+ valuename +" from cey_sys_operator where vch_operid = '%2' ")
                         .arg(operid));
        query.next();
        var = query.recordValue(valuename);
    }

    return var;
}

void w_scr_dish_switch_detail::set_cey_sys_operator(const QString &operid, const QString &valuename, const QVariant &var)
{
    lds_query query;
    if(operid  == "0000") {
        n_func::f_set_sysparm("system_" + valuename, var);
    } else {;
        query.execUpdate("cey_sys_operator", valuename, var, qrtEqual("vch_operid", operid));
    }
}

bool w_scr_dish_switch_detail::handover_oper_check()
{
    //是否需要有交班人
    lds_query query;
    QString ls_operid = public_sql::gs_operid;

    lds_dialog_input dialog(this);
    dialog.showSecondLineEdt();
    dialog.ui->label->setText(tr("接班人"));
    dialog.ui->label_2->setText(tr("密码"));
    dialog.ui->lineEdit_2->setEchoMode(QLineEdit::Password);

    if(n_func::f_get_sysparm("handover_oper")=="1") {
        QString errtext;
        switch(0){
        case 0:
            dialog.setWindowTitle(tr("接班人"));
            if(QDialog::Rejected==lds_roundeddialog_rect_align(&dialog).exec()){
                return false;
            }
            ls_operid = dialog.ui->lineEdit->text();
        case 1:
            if(ls_operid == public_sql::gs_operid){
                errtext=tr("接班人")+tr("不能和交班人为同一人");
                break;
            }
            if(ls_operid.isEmpty()){
                errtext=tr("操作员代码不能为空");
                break;
            }
            if(ls_operid==public_sql::gs_operid){
                errtext=tr("接班人和交班人不能为同一人");
                break;
            }
        case 2:
            if(ls_operid=="0000"){
                if(dialog.ui->lineEdit_2->text()!=n_func::f_get_sysparm("system_pass","0000")){//n_func::f_m_get_parameter(this,"system", "0000")){
                    errtext=tr("操作密码错误");
                }
                break;
            }
        case 3:
            query.execSelect(QString("select vch_operID,vch_password from cey_sys_operator where ch_flag = 'Y' and vch_operID ='%1' ")
                             .arg(ls_operid));
            if(!query.next()){
                errtext=tr("操作员代码不存在");
                break;
            }

            query.execSelect(QString("select vch_operID,vch_password from cey_sys_operator where ch_flag = 'Y' and vch_operID ='%1' and vch_password='%2' ")
                             .arg(ls_operid)
                             .arg(dialog.ui->lineEdit_2->text())
                             );
            if(!query.next()){
                errtext=tr("操作密码错误");
                break;
            }
        }
        if(!n_func::f_check_bus_grant_else_authority(ls_operid, "0217", this, this->windowTitle())){
            errtext = tr("指定的操作员没有接班权限");
        }
        if(!errtext.isEmpty()){
            lds_messagebox::warning(this, MESSAGE_TITLE_VOID,errtext);
            return false;
        }
        new_operid = dialog.ui->lineEdit->text();
        new_operpwd = dialog.ui->lineEdit_2->text();

        //接班人
        if(new_operid == "0000") {
            ui->tableWidget->item(1,3)->setText(tr("system(0000)"));
        } else {
            ui->tableWidget->item(1,3)->setText(new_operid );
        }
    }

    return true;
}

int w_scr_dish_switch_detail::get_max_temp_int_id_insert_cey_u_handover_master(bool *ok)
{
    lds_query query;
    int int_id = 0;
    if(ok) *ok = false;

    query.execSelect(QString(" select int_id from cey_u_handover_master where int_id < 0 and vch_operID = '%1' ")
                     .arg(public_sql::gs_operid));
    if(query.next()) {
        int_id = query.recordValue("int_id").toInt();
        if(ok) *ok = true;
    } else {
        //小于0的查询最小的，后台报表不会查询
        query.execSelect(QString(" select min(int_id) from cey_u_handover_master where int_id < 0"));
        query.next();
        int_id = query.recordValue(0).toInt() - 1;
        bool f = query.execInsert("cey_u_handover_master", qrtVariantPairList()
                                  <<qrtVariantPair("int_id", int_id)
                                  <<qrtVariantPair("vch_operID", public_sql::gs_operid)
                                  <<qrtVariantPair("dt_begin", n_func::f_get_sysdatetime())
                                  <<qrtVariantPair("dt_end", n_func::f_get_sysdatetime())
                                  <<qrtVariantPair("vch_memo2", "取款，交班后删除")
                                  );
        if(ok) *ok = f;
    }

    return int_id;
}

void w_scr_dish_switch_detail::update_temp_handover()
{
    lds_query query;
    query.execSelect(QString(" select sum(a.num_amount), count(a.num_amount) from cey_u_handover_total a, cey_u_handover_master b  where a.int_id = b.int_id and b.int_id < 0 and b.vch_operID = '%1' ")
                     .arg(public_sql::gs_operid));
    query.next();
    ui->pushButton_fetch->setNumber(query.recordValue(0).toDouble());
    ui->pushButton_fetch_record->setNumber(query.recordValue(1).toDouble());
    query.execSelect(QString(" select vch_memo from cey_u_handover_master b  where int_id < 0 and b.vch_operID = '%1' ")
                     .arg(public_sql::gs_operid));
    query.next();
    ui->lineEdit_vch_memo->setText(query.recordValue("vch_memo").toString());
}

void w_scr_dish_switch_detail::toInitCommitData()
{
    if(n_func::f_get_sysparm("show_info_before_shift", "0") == "1") {//显示交班信息，交班前
        QString errstring;
        lds_query::tran_saction();
        commitData(errstring, int_id);
        refresh_tablewidget(int_id);
        lds_query::roll_back();
    }
}

QString w_scr_dish_switch_detail::handover_tmp_vch_memo()
{
    lds_query query;
    query.execSelect(QString(" select vch_memo from cey_u_handover_master b  where int_id < 0 and b.vch_operID = '%1' ")
                     .arg(public_sql::gs_operid));
    query.next();
    return query.recordValue("vch_memo").toString();
}

void w_scr_dish_switch_detail::print_by_int_id(int int_id)
{
    blockIns printins;
    lds_query query;
    lds_query query2;
    QString date0;
    QString date1;
    QString vch_operid;
    QString vch_accept;

    int columnwidth1 = 16;
    int columnwidth2 = 16;
    query.execSelect(QString("SELECT int_paperwidth FROM cey_bt_kitchen_print where vch_printip = '%1'  ").arg(backheader::print_def_path()));
    if(query.next()) {
        columnwidth1 = query.recordValue("int_paperwidth").toInt() - columnwidth2;
        if(columnwidth1 < 16) columnwidth1 = 16;
    }

    query.execSelect(QString("select vch_operID, dt_begin, dt_end, vch_accept, vch_memo FROM hddpos.cey_u_handover_master where int_id = '%1' ")
                     .arg(int_id));
    query.next();
    date0 = query.recordValue("dt_begin").toDateTime().toString(yyyyMMddhhmmss);
    date1 = query.recordValue("dt_end").toDateTime().toString(yyyyMMddhhmmss);
    vch_operid = query.recordValue("vch_operID").toString();
    vch_accept = query.recordValue("vch_accept").toString();

    printins.append(backheader::printer.drawText("a00,s11", tr("交班对账表") +"\n","a10,s00"));
    printins.append(backheader::printer.drawText("", tr("交班员")+":"+vch_operid +"     "+tr("接班人")+":"+vch_accept+"\n" ));
    printins.append(backheader::printer.drawText("", tr("开始时间")+":"+date0+"\n"));
    printins.append(backheader::printer.drawText("", tr("交班时间")+":"+date1+"\n"));
    printins.append(backheader::printer.drawText("", tr("备注")+":"+ query.recordValue("vch_memo").toString() +"\n"));


    //收银流水
    if(lds::sysconf->value("w_scr_dish_switch_detail/isprintdetail", false).toBool()) {
        bool print_header = false;
        query.execSelect(QString("select a.ch_payno, sum(a.num_realamount)  "
                                 " from cey_u_checkout_detail a , cey_u_checkout_master b where ( a.ch_payno =b.ch_payno ) and ( b.ch_state ='Y' ) "
                                 " and ( b.vch_operID ='%1' ) and ( b.dt_operdate >= '%2' and b.dt_operdate <= '%3' ) and "
                                 ///免单，抹零，优惠，挂账不会显示
                                 " a.ch_paymodeno <> '' and a.ch_paymodeno <>'XX' and a.ch_paymodeno <>'YY' and a.ch_paymodeno <>'ZZ' and a.ch_paymodeno <>'11' "
                                 " group by a.ch_payno order by a.ch_payno asc"
                                 )
                         .arg(vch_operid)
                         .arg(date0)
                         .arg(date1)
                         );

        while(query.next()) {
            if(!print_header) {
                printins.append(backheader::printer.drawText("a00,s00", "--"+tr("收银流水")+"--\n", "a10,s00"));
                printins.append(backheader::printer.drawText("-"));
                printins.append(backheader::printer.drawText("", tr("项目"), columnwidth1, Qt::AlignLeft));
                printins.append(backheader::printer.drawText("", tr("金额")+"\n", columnwidth2, Qt::AlignRight));
                print_header = true;
            }
            printins.append(backheader::printer.drawText("-"));
            QString ch_payno = query.recordValue("ch_payno").toString();
            double pay_total = query.recordValue(1).toDouble();


            printins.append(backheader::printer.drawText("", ch_payno, columnwidth1, Qt::AlignLeft));
            printins.append(backheader::printer.drawText("", lds::septNumber( pay_total , 2)+"\n", columnwidth2, Qt::AlignRight));
            query2.execSelect(QString("Select a.ch_payno, "
                                      " (select aa.vch_paymodename from cey_bt_paymode aa where aa.ch_paymodeno=a.ch_paymodeno) as paymodename ,"
                                      " a.ch_paymodeno,  a.num_realamount, a.num_face"
                                      " from cey_u_checkout_detail a where  a.ch_payno = '%1' and "
                                      ///免单，抹零，优惠，挂账不会显示
                                      " a.ch_paymodeno <> '' and a.ch_paymodeno <>'XX' and a.ch_paymodeno <>'YY' and a.ch_paymodeno <>'ZZ' and a.ch_paymodeno <>'11' ")
                              .arg(ch_payno)
                              );
            while(query2.next()) {
                printins.append(backheader::printer.drawText("", query2.recordValue("paymodename").toString(), columnwidth1, Qt::AlignLeft));
                printins.append(backheader::printer.drawText("", lds::septNumber( query2.recordValue("num_realamount").toDouble() , 2)+"\n", columnwidth2, Qt::AlignRight));
            }
        }
    }

    //收银汇总
    printins.append(backheader::printer.drawText("a00,s00", "--"+tr("收银汇总")+"--\n", "a10,s00"));
    printins.append(backheader::printer.drawText("-"));

    query.execSelect(QString("select vch_item, num_amount, vch_item2, num_amount2  FROM cey_u_handover_total where int_id = '%1' order by int_row asc ")
                     .arg(int_id));
    while(query.next()){
        QString pre;
        QString vch_item = query.recordValue("vch_item").toString();
        QString vch_item2 = query.recordValue("vch_item2").toString();
        if(vch_item2.contains("X")) {
             vch_item = lds::size_g_of_u_left_justified(vch_item, 18) + "x" +query.recordValue("num_amount2").toString();
        }
        if(vch_item2 == "L") {//线
            if(vch_item.length() > 0) {
                printins.append(backheader::printer.drawText("a00,s00", "--" + vch_item + "--\n", "a10,s00"));
            }
            printins.append(backheader::printer.drawText("-"));
            continue;
        }
        if(vch_item2.contains("P")) {
            printins.append(backheader::printer.drawText("a10,s00"));
        }

        if(vch_item2.contains("Y")) {
            printins.append(backheader::printer.drawText("a10,s00"));
            int Ycount = vch_item2.count("Y");
            for(int k = 0; k < Ycount; k++) {
                pre += "  ";
            }
        }
        printins.append(backheader::printer.drawText("", pre + vch_item, columnwidth1, Qt::AlignLeft));
        if(vch_item2.contains("N")) {
            printins.append(backheader::printer.drawText("", "\n"));
        } else {
            printins.append(backheader::printer.drawText("", lds::septNumber( query.recordValue("num_amount").toDouble() , 2) + "\n", columnwidth2, Qt::AlignRight));
        }
        printins.append(backheader::printer.drawText("a10,s00"));
    }

    //打印后面
    printins.append(backheader::printer.drawText("-"));
    printins.append(backheader::printer.drawText("a10,s00", tr("签名") + "\n", columnwidth1, Qt::AlignLeft));
    printins.append(backheader::printer.drawText("a10,s00"));
    printins.append(backheader::printer.drawTextEnd("1B6405 1D5601", ""));//走纸5x， 切纸
    printins = backheader::printer.setPrintCount(printins, lds::sysconf->value("w_scr_dish_switch_detail/printcount", 1).toDouble());

    backheader::asyn_dataprint(printins);
}

void w_scr_dish_switch_detail::refresh_tablewidget(int int_id)
{
    lds_query query;
    query.execSelect(QString("select vch_operID, dt_begin, dt_end, vch_accept, vch_memo FROM hddpos.cey_u_handover_master where int_id = '%1' ")
                     .arg(int_id));
    query.next();
    //交班人
    ui->tableWidget->item(1,1)->setText(query.recordValue("vch_operID").toString());
    //接班人
    ui->tableWidget->item(1,3)->setText(query.recordValue("vch_accept").toString());
    //开始时间
    ui->tableWidget->item(2,1)->setText(query.recordValue("dt_begin").toDateTime().toString(yyyyMMddhhmmss));
    //交班时间
    ui->tableWidget->item(3,1)->setText(query.recordValue("dt_end").toDateTime().toString(yyyyMMddhhmmss));
    //备注
    if(query.recordValue("vch_memo").toString().length() > 0) {
        ui->tableWidget->item(4,1)->setText(query.recordValue("vch_memo").toString());
        ui->tableWidget->item(4,0)->setText(tr("备注"));
    }

    //
    int currow  = 4;//当前所在行4
    while(ui->tableWidget->rowCount() > 5)
        ui->tableWidget->removeRow(5);
    QTableWidgetItem *tmpitem;
    query.execSelect(QString("select vch_item, num_amount, vch_item2, num_amount2 FROM cey_u_handover_total where int_id = '%1' order by int_row asc ")
                     .arg(int_id));
    while(query.next()){
        QString vch_item2 = query.recordValue("vch_item2").toString();
        currow ++;
        if(ui->tableWidget->rowCount() <= currow) {
            currow = ui->tableWidget->rowCount();
            ui->tableWidget->insertRow(currow);
            ui->tableWidget->setSpan(currow, 0, 1, 2);
        }
        //
        QString pre;
        for(int k = 0, count = vch_item2.count("Y"); k < count; k++) {
            pre += "  ";
        }
        //
        ui->tableWidget->setItem(currow, 0 , tmpitem=new QTableWidgetItem(pre+query.recordValue("vch_item").toString() ));
        //
        if(vch_item2.contains("X")) {
            ui->tableWidget->setItem(currow, 2 , tmpitem=new QTableWidgetItem("x" + query.recordValue("num_amount2").toString()));
        }
        //
        if(!vch_item2.contains("N")
                && !vch_item2.contains("L")) {
            ui->tableWidget->setItem(currow, 3 , tmpitem=new QTableWidgetItem(lds::septNumber( query.recordValue("num_amount").toDouble(), 2)));{tmpitem->setTextAlignment(Qt::AlignRight);}
        }
    }
}

void w_scr_dish_switch_detail::tofinish()
{
    this->accept();
}

void w_scr_dish_switch_detail::toswitch()
{
    if(false == handover_oper_check()) {
        return ;
    }
    //2
    QString errstring;
    lds_query::tran_saction();
    if(!commitData(errstring, int_id, false)) {
        lds_query::roll_back();
        qDebug() << "errstring:"  << errstring;
        lds_messagebox::warning(this, MESSAGE_TITLE_VOID, tr("交班失败"));
        //
        return;
    }
    lds_query::com_mit();
    //3
    if(ui->checkBox->checkedValue().toBool()) {
        print_by_int_id(int_id);
    }
    //4
    ui->pushButton_print->show();
    ui->pushButton_finish->show();
    ui->pushButton_switch->hide();
    ui->pushButton_exit->hide();
    ui->pushButton_set->hide();

    lds_messagebox::information(this, MESSAGE_TITLE_VOID, tr("交班成功"));
    ui->pushButton_finish->setFocus();
    refresh_tablewidget(int_id);

    ui->pushButton_cash_register->setEnabled(false);
    ui->pushButton_printcount->setEnabled(false);
}

void w_scr_dish_switch_detail::toexit()
{
    this->reject();
}

void w_scr_dish_switch_detail::tosaveupload(bool f)
{
    lds::sysconf->setValue("w_scr_dish_switch_detail/is_upload_flowdata", f);
}

void w_scr_dish_switch_detail::to_set_print_count()
{
    lds_dialog_input_double inputdialog(this);
    inputdialog.ui->label->setText(tr("打印份数"));
    inputdialog.ui->lineEdit->setDecimals(0);
    inputdialog.ui->lineEdit->setValue(ui->pushButton_printcount->number());
    inputdialog.setWindowTitle(this->windowTitle());
    if(QDialog::Accepted== lds_roundeddialog_rect_align(&inputdialog).exec()){
        lds::sysconf->setValue("w_scr_dish_switch_detail/printcount", inputdialog.ui->lineEdit->value());
        ui->pushButton_printcount->setNumber(inputdialog.ui->lineEdit->value());
    }
}

void w_scr_dish_switch_detail::to_set_cash_register()
{
    if(get_cey_sys_operator(public_sql::gs_operid, "ch_shift_config_flag").toString() == "Y") {
        lds_messagebox::warning(this, MESSAGE_TITLE_VOID, tr("同一操作员只可以修改一次备用金"));
        return;
    }
    if(0 == lds_messagebox::question(this, MESSAGE_TITLE_VOID, tr("同一操作员只可以修改一次备用金"), tr("继续"), tr("取消"))) {
        lds_dialog_input_double inputdialog(this);
        inputdialog.ui->label->setText(tr("备用金"));
        inputdialog.ui->lineEdit->setValue(get_cey_sys_operator(public_sql::gs_operid, "num_shift_cash_register").toDouble());
        inputdialog.setWindowTitle(this->windowTitle());
        if(QDialog::Accepted== lds_roundeddialog_rect_align(&inputdialog).exec()){
            set_cey_sys_operator(public_sql::gs_operid, "ch_shift_config_flag", "Y");
            set_cey_sys_operator(public_sql::gs_operid, "num_shift_cash_register", inputdialog.ui->lineEdit->value());
            ui->pushButton_cash_register->setNumber(get_cey_sys_operator(public_sql::gs_operid, "num_shift_cash_register").toDouble());
        }
    }
}

void w_scr_dish_switch_detail::tosaveprintdetail(bool f)
{
    lds::sysconf->setValue("w_scr_dish_switch_detail/isprintdetail", f);
}

void w_scr_dish_switch_detail::tosave_cash_fee_keep_to_accept(bool f)
{
    lds::sysconf->setValue("w_scr_dish_switch_detail/cash_fee_keep_to_accept", f);
}

void w_scr_dish_switch_detail::toprint()
{
    if(int_id != -1) {
        print_by_int_id(int_id);
    }
}

void w_scr_dish_switch_detail::tofetch()
{
    lds_query query;
    lds_dialog_input_double inputdialog(this);
    inputdialog.ui->label->setText(tr("金额"));
    inputdialog.setWindowTitle(tr("取钱"));
    if(QDialog::Accepted== lds_roundeddialog_rect_align(&inputdialog).exec()) {
        lds_query::tran_saction();
        bool ok;
        int int_id = get_max_temp_int_id_insert_cey_u_handover_master(&ok);
        if(false == ok) {
            lds_query::roll_back();
            lds_messagebox::warning(this, MESSAGE_TITLE_VOID, tr("操作失败"));
            return;
        }
        query.execSelect(QString(" select max(int_row) from cey_u_handover_total where int_id = '%1' ")
                         .arg(int_id));
        query.next();
        int int_row = query.recordValue(0).toInt() + 1;
        if(false == query.execInsert("cey_u_handover_total", qrtVariantPairList()
                                     <<qrtVariantPair("int_id", int_id)
                                     <<qrtVariantPair("int_row", int_row)
                                     <<qrtVariantPair("vch_item", tr("取款") + QString::number(int_row))
                                     <<qrtVariantPair("num_amount", inputdialog.ui->lineEdit->value())
                                     )) {
            lds_query::roll_back();
            lds_messagebox::warning(this, MESSAGE_TITLE_VOID, tr("操作失败"));
            return;
        }
        lds_query::com_mit();
        lds_messagebox::information(this, MESSAGE_TITLE_VOID, tr("操作成功"));

        update_temp_handover();
    }
}

void w_scr_dish_switch_detail::tofetchrecord()
{
    //
    lds_query query;
    query.execSelect(QString(" select int_id from cey_u_handover_master where int_id < 0 and vch_operID = '%1' ")
                     .arg(public_sql::gs_operid));
    query.next();
    int int_id = query.recordValue("int_id").toInt();
    //
    lds_model_sqltablemodel tablemodel;
    tablemodel.setTable("cey_u_handover_total");
    tablemodel.save_set_header("vch_item", tr("项目"));
    tablemodel.save_set_header("num_amount", tr("值"));
    tablemodel.setFilter(QString("int_id = '%1' ").arg(int_id));

    QDialog dialog(this);
    lds_tableView tableview(&dialog);
    QPushButton btnOk(tr("退出"), &dialog);
    dialog.resize(300, 400);
    tableview.setGeometry(10, 10, 280, 340);
    btnOk.setGeometry(210, 360, 80, 30);
    dialog.setWindowTitle(tr("操作员") +":" + public_sql::gs_operid);
    connect(&btnOk, SIGNAL(clicked(bool)), &dialog, SLOT(reject()));

    tableview.setModel(&tablemodel);
    tableview.rePecifyHeader(tablemodel.get_save_set_header_saver_column_list());
    tableview.horizontalHeader()->setStretchLastSection(true);

    lds_roundeddialog_rect_align(&dialog).exec();
}

void w_scr_dish_switch_detail::update_shift_memo()
{
    lds_query query;
    lds_query::tran_saction();
    bool ok;
    int int_id = get_max_temp_int_id_insert_cey_u_handover_master(&ok);
    if(false == ok) {
        lds_query::roll_back();
        lds_messagebox::warning(this, MESSAGE_TITLE_VOID, tr("操作失败"));
        return;
    }
    if(false == query.execUpdate("cey_u_handover_master", "vch_memo", ui->lineEdit_vch_memo->text(), qrtEqual("int_id", int_id))) {
        lds_query::roll_back();
        lds_messagebox::warning(this, MESSAGE_TITLE_VOID, tr("操作失败"));
        return;
    }
    lds_query::com_mit();
}
