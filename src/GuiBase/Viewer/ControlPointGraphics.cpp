#include "ControlPointGraphics.h"
#include <QGraphicsScene>

ControlPointGraphics::ControlPointGraphics( qreal x, qreal y, qreal w, qreal h,
                                            QGraphicsItem* parent ) :
    QGraphicsEllipseItem( x, y, w, h, parent ),
    m_movingAreaSet( false ) {}

ControlPointGraphics::~ControlPointGraphics() {}

QVariant ControlPointGraphics::itemChange( GraphicsItemChange change, const QVariant& value ) {
    // value seems to contain position relative start of moving
    if ( change == ItemPositionChange )
    {
        QPointF newPos = value.toPointF();
        QRectF rect = m_movingAreaSet ? m_movingArea : scene()->sceneRect();
        if ( !rect.contains( newPos ) )
        {
            // Keep the item inside the scene rect.
            newPos.setX( qMin( rect.right(), qMax( newPos.x(), rect.left() ) ) );
            newPos.setY( qMin( rect.bottom(), qMax( newPos.y(), rect.top() ) ) );
        }
        emit itemMoved( pos() );
        return newPos;
    }
    return QGraphicsEllipseItem::itemChange(
        change, value ); // i allso tried to call this before the emiting
}

void ControlPointGraphics::setMovingArea( const QRectF& rect ) {
    m_movingArea = rect;
    m_movingAreaSet = true;
}
