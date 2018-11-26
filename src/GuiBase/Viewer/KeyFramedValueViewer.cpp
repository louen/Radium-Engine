#include "KeyFramedValueViewer.h"
#include "ui_KeyFramedValueViewer.h"

#include <fstream>
#include <iostream>

#include <QFileDialog>

#define POINT_MOVING_AREA_X 0
#define POINT_MOVING_AREA_Y -50
#define POINT_MOVING_AREA_H 100

#define FRAME_INTERVAL 20
#define VALUE_INTERVAL 10

KeyFramedValueViewer::KeyFramedValueViewer( const KeyFramedValue& kfv, int maxFrames,
                                            QWidget* parent ) :
    QWidget( parent ),
    m_currentFrame( 0 ),
    m_currentEditFrame( 0 ),
    m_value( kfv ),
    m_valueSave( kfv ),
    m_ui( new Ui::KeyFramedValueViewer ) {
    m_ui->setupUi( this );
    m_ui->m_valueName->setText( QString::fromStdString( m_value.m_name ) );
    m_ui->m_minBound_dsb->setValue( m_value.m_bounds( 0 ) );
    m_ui->m_maxBound_dsb->setValue( m_value.m_bounds( 1 ) );

    m_maxFrames = maxFrames == 0 ? m_value.m_keyFrames.rbegin()->first : maxFrames;

    m_scene = new QGraphicsScene( this );
    m_ui->graphicsView->setScene( m_scene );
    m_ui->graphicsView->setBackgroundBrush(
        QBrush( QColor( 150, 150, 150, 255 ), Qt::SolidPattern ) );

    m_points.resize( m_maxFrames + 1 );
    for ( uint i = 0; i <= m_maxFrames; ++i )
    {
        m_points[i] = new ControlPointGraphics( 0, 0, 10, 10 );
        m_points[i]->setFlag( QGraphicsItem::ItemIsMovable, true );
        m_points[i]->setFlag( QGraphicsItem::ItemIsSelectable, true );
        m_points[i]->setFlag( QGraphicsItem::ItemSendsScenePositionChanges, true );
        m_scene->addItem( m_points[i] );
        connect( m_points[i], &ControlPointGraphics::itemMoved,
                 [this, i]( const QPointF& pos ) { on_pointMoved( i, pos ); } );
    }

    QPen redPen( Qt::red );
    redPen.setWidth( 2 );
    m_curve.resize( m_maxFrames );
    for ( auto& line : m_curve )
    {
        line = new QGraphicsLineItem();
        line->setPen( redPen );
        m_scene->addItem( line );
    }

    QPen greyPen( QColor( 100, 100, 100, 255 ) );
    greyPen.setWidth( 1 );
    m_grid.resize( m_maxFrames + 1 + VALUE_INTERVAL + 1 );
    for ( auto& line : m_grid )
    {
        line = new QGraphicsLineItem();
        line->setPen( greyPen );
        m_scene->addItem( line );
    }

    setupKeyFrames();
    updateCurve();
    updateGrid();
    setCurrentFrame( m_currentFrame );
    // update ui
    m_ui->m_currentTime_l->setText( QString::number( 0 ) );
    m_ui->m_currentValue_dsb->blockSignals( true );
    m_ui->m_currentValue_dsb->setValue( m_value.m_keyFrames.begin()->second );
    m_ui->m_currentValue_dsb->setMinimum( m_value.m_bounds( 0 ) );
    m_ui->m_currentValue_dsb->setMaximum( m_value.m_bounds( 1 ) );
    m_ui->m_currentValue_dsb->blockSignals( false );
}

KeyFramedValueViewer::~KeyFramedValueViewer() {
    delete m_ui;
}

const KeyFramedValue& KeyFramedValueViewer::getKeyFramedValue() const {
    return m_value;
}

void KeyFramedValueViewer::setKeyFramedValue( const KeyFramedValue& kfv ) {
    m_value = kfv;
    m_valueSave = kfv;
    m_ui->m_valueName->setText( QString::fromStdString( m_value.m_name ) );
    m_ui->m_minBound_dsb->setValue( m_value.m_bounds( 0 ) );
    m_ui->m_maxBound_dsb->setValue( m_value.m_bounds( 1 ) );

    // todo: deal with new max number of frames

    setupKeyFrames();
    updateCurve();
    updateGrid();
    setCurrentFrame( m_currentFrame );
    // update ui
    m_ui->m_currentTime_l->setText( QString::number( m_currentFrame ) );
    m_ui->m_currentValue_dsb->blockSignals( true );
    m_ui->m_currentValue_dsb->setValue( getValueFromFrame( m_currentFrame ) );
    m_ui->m_currentValue_dsb->blockSignals( false );
}

void KeyFramedValueViewer::setCurrentFrame( int frame ) {
    if ( frame > m_maxFrames || frame == m_currentFrame )
    {
        return;
    }
    // uncolor current
    QPen greyPen( QColor( 30, 30, 30, 255 ) );
    greyPen.setWidth( 1 );
    m_grid[m_currentFrame]->setPen( greyPen );
    // change current
    m_currentFrame = frame;
    m_currentEditFrame = frame;
    // color current
    QPen yellowPen( Qt::yellow );
    yellowPen.setWidth( 2 );
    m_grid[m_currentFrame]->setPen( yellowPen );
    // update ui
    m_ui->m_currentTime_l->setText( QString::number( frame ) );
    m_ui->m_currentValue_dsb->blockSignals( true );
    m_ui->m_currentValue_dsb->setValue( getValueFromFrame( frame ) );
    m_ui->m_currentValue_dsb->blockSignals( false );
}

void KeyFramedValueViewer::setMaxFrame( int maxFrame ) {
    // todo: deal with new max number of frames
}

void KeyFramedValueViewer::on_m_load_pb_clicked() {
    emit loadValue();
}

void KeyFramedValueViewer::on_m_save_pb_clicked() {
    emit saveValue();
}

void KeyFramedValueViewer::updateCurve() {
    for ( uint i = 0; i < m_curve.size(); ++i )
    {
        m_curve[i]->setLine( m_points[i]->x() + 5, m_points[i]->y() + 5, m_points[i + 1]->x() + 5,
                             m_points[i + 1]->y() + 5 );
    }
}

void KeyFramedValueViewer::updateGrid() {
    const auto y0 = value_to_graphics( m_value.m_bounds( 0 ) );
    const auto y1 = value_to_graphics( m_value.m_bounds( 1 ) );
    for ( uint i = 0; i < m_points.size(); ++i )
    {
        auto x = frame_to_graphics( i );
        m_grid[i]->setLine( x, y0, x, y1 );
    }
    const Scalar diff = m_value.m_bounds( 1 ) - m_value.m_bounds( 0 );
    const auto x0 = frame_to_graphics( 0 );
    const auto x1 = frame_to_graphics( m_maxFrames );
    for ( uint i = 0; i < VALUE_INTERVAL + 1; ++i )
    {
        auto y = value_to_graphics( m_value.m_bounds( 0 ) + Scalar( i ) * diff / VALUE_INTERVAL );
        m_grid[m_points.size() + i]->setLine( x0, y, x1, y );
    }
}

Scalar KeyFramedValueViewer::value_from_graphics( Scalar y ) {
    return -( y + POINT_MOVING_AREA_Y ) / POINT_MOVING_AREA_H *
               ( m_value.m_bounds( 1 ) - m_value.m_bounds( 0 ) ) +
           m_value.m_bounds( 0 );
}

Scalar KeyFramedValueViewer::value_to_graphics( Scalar value ) {
    return -POINT_MOVING_AREA_Y - ( value - m_value.m_bounds( 0 ) ) /
                                      ( m_value.m_bounds( 1 ) - m_value.m_bounds( 0 ) ) *
                                      POINT_MOVING_AREA_H;
}

Scalar KeyFramedValueViewer::frame_to_graphics( int frame ) {
    return POINT_MOVING_AREA_X + frame * FRAME_INTERVAL;
}

Scalar KeyFramedValueViewer::getValueFromFrame( int frame ) {
    Scalar value;
    auto first = m_value.m_keyFrames.begin();
    auto last = m_value.m_keyFrames.rbegin();
    auto end = m_value.m_keyFrames.end();
    auto lower = m_value.m_keyFrames.lower_bound( frame );
    auto upper = m_value.m_keyFrames.upper_bound( frame );
    if ( upper == first )
    {
        value = first->second;
    } else if ( lower == end )
    {
        value = last->second;
    } else if ( lower == upper )
    {
        // FIXME: bug here!
        auto a = *lower;
        auto b = *( --lower );
        // interpolate between those
        Scalar t = Scalar( frame - b.first ) / ( a.first - b.first );
        value = t * a.second + ( 1.f - t ) * b.second;
    } else
    { value = lower->second; }
    return value;
}

void KeyFramedValueViewer::on_pointMoved( int i, QPointF pos ) {
    m_currentEditFrame = i;
    updateCurve();
    // update keyframes
    if ( m_value.m_keyFrames.find( i ) == m_value.m_keyFrames.end() )
    {
        QBrush blueBrush( Qt::blue );
        QPen bluePen( Qt::blue );
        bluePen.setWidth( 2 );
        m_points[i]->setPen( bluePen );
        m_points[i]->setBrush( blueBrush );
    }
    m_value.m_keyFrames[i] = value_from_graphics( pos.y() );
    setupKeyFrames();
    // update ui
    m_ui->m_currentTime_l->setText( QString::number( i ) );
    m_ui->m_currentValue_dsb->blockSignals( true );
    m_ui->m_currentValue_dsb->setValue( m_value.m_keyFrames[i] );
    m_ui->m_currentValue_dsb->blockSignals( false );
    // signal
    emit valueEdited();
}

void KeyFramedValueViewer::on_m_minBound_dsb_valueChanged( double arg1 ) {
    m_value.m_bounds( 0 ) = arg1;
    setupKeyFrames();
    updateCurve();
    m_ui->m_currentValue_dsb->setMinimum( m_value.m_bounds( 0 ) );
}

void KeyFramedValueViewer::on_m_maxBound_dsb_valueChanged( double arg1 ) {
    m_value.m_bounds( 1 ) = arg1;
    setupKeyFrames();
    updateCurve();
    m_ui->m_currentValue_dsb->setMaximum( m_value.m_bounds( 1 ) );
}

void KeyFramedValueViewer::on_m_currentValue_dsb_valueChanged( double arg1 ) {
    // update keyframes
    if ( m_value.m_keyFrames.find( m_currentEditFrame ) == m_value.m_keyFrames.end() )
    {
        QBrush blueBrush( Qt::blue );
        QPen bluePen( Qt::blue );
        bluePen.setWidth( 2 );
        m_points[m_currentEditFrame]->setPen( bluePen );
        m_points[m_currentEditFrame]->setBrush( blueBrush );
    }
    m_value.m_keyFrames[m_currentEditFrame] = arg1;
    setupKeyFrames();
    updateCurve();
    // signal
    emit valueEdited();
}

void KeyFramedValueViewer::setupKeyFrames() {
    QBrush blackBrush( Qt::black );
    QBrush blueBrush( Qt::blue );
    QPen blackPen( Qt::black );
    blackPen.setWidth( 2 );
    QPen bluePen( Qt::blue );
    bluePen.setWidth( 2 );

    for ( uint i = 0; i < m_points.size(); ++i )
    {
        // get keyFrames at i
        Scalar value = getValueFromFrame( i );
        bool isFramed = m_value.m_keyFrames.find( i ) != m_value.m_keyFrames.end();
        // add point
        m_points[i]->blockSignals( true );
        m_points[i]->setMovingArea(
            QRectF( frame_to_graphics( i ) - 5, POINT_MOVING_AREA_Y - 5, 1, POINT_MOVING_AREA_H ) );
        m_points[i]->setPos( frame_to_graphics( i ) - 5, value_to_graphics( value ) - 5 );
        m_points[i]->setPen( isFramed ? bluePen : blackPen );
        m_points[i]->setBrush( isFramed ? blueBrush : blackBrush );
        m_points[i]->blockSignals( false );
    }
}

void KeyFramedValueViewer::on_m_reset_pb_clicked() {
    m_value = m_valueSave;
    setupKeyFrames();
    updateCurve();
    m_ui->m_currentTime_l->setText( QString::number( m_currentFrame ) );
    m_ui->m_currentValue_dsb->blockSignals( true );
    m_ui->m_currentValue_dsb->setValue( getValueFromFrame( m_currentFrame ) );
    m_ui->m_currentValue_dsb->blockSignals( false );
    // signal
    emit valueEdited();
}
