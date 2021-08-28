#include "videoitem.h"
#include "ui_videoitem.h"
#include<QPainter>

VideoItem::VideoItem(QWidget *parent) :
    QWidget(parent),m_defaultImg(":/tx/12.png"),
    ui(new Ui::VideoItem)
{
    ui->setupUi(this);

    m_timer = new QTimer;
    connect( m_timer , SIGNAL(timeout())
             ,this , SLOT(slot_checkTime() ) );
    m_timer->start( 500 );

    m_lastTime = QTime::currentTime();


}

VideoItem::~VideoItem()
{
    delete ui;

    if( m_timer )
    {
        m_timer->stop();
        delete m_timer;
    }
}

//设置信息
void VideoItem::setInfo(int id, QString name)
{
    m_id = id ;
    m_name = name ;

    //ui
    ui->lb_name->setText( QString("用户:%1").arg(m_name) );
}

//设置图片
void VideoItem::slot_setOneImage(QImage  img)
{
    m_lastTime = QTime::currentTime();
    m_image = img;

    this->repaint(); // 直接调用重绘 --> 到绘图事件
}

//绘图事件
void VideoItem::paintEvent(QPaintEvent *event)
{
    //画黑背景
    QPainter  painter( this );
    painter.setBrush( Qt::black );
    painter.drawRect( 0,0, this->width() , this->height());

    //画图片
    if( m_image.size().height()<= 0  ) return;

    // 加载图片用 QImage , 画图使用QPixmap
    // 图片缩放  scaled
    QPixmap  pixmap =  QPixmap::fromImage(
                m_image.scaled( QSize( this->width() , this->height() - ui->lb_name->height())
                                , Qt::KeepAspectRatio ,Qt::FastTransformation ));

    //画的位置
    int x = this->width() - pixmap.width();
    int y = this->height() - pixmap.height() - ui->lb_name->height();
    x = x /2;
    y = ui->lb_name->height() + y/2;

    painter.drawPixmap( QPoint(x,y) , pixmap );
}

void VideoItem::mousePressEvent(QMouseEvent *event)
{
    Q_EMIT SIG_itemClicked(m_id);
}

//检查图片
void VideoItem::slot_checkTime()
{
    if( m_lastTime .secsTo( QTime::currentTime() ) > 3 )
    {
        m_lastTime = QTime::currentTime();
        m_image = m_defaultImg;
        this->repaint();
    }
}

