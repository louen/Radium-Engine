#ifndef CONTROLPOINTGRAPHICS_H
#define CONTROLPOINTGRAPHICS_H

#include <QGraphicsEllipseItem>

class ControlPointGraphics : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
  public:
    explicit ControlPointGraphics( qreal x, qreal y, qreal w, qreal h, QGraphicsItem* parent = 0 );
    ~ControlPointGraphics();

    QVariant itemChange( GraphicsItemChange change, const QVariant& value ) override;

    void setMovingArea( const QRectF& rect );

  signals:
    void itemMoved( QPointF p );

  private:
    QRectF m_movingArea;
    bool m_movingAreaSet;
};

#endif // CONTROLPOINTGRAPHICS_H
