#ifndef KEYFRAMEDVALUEVIEWER_H
#define KEYFRAMEDVALUEVIEWER_H

#include <map>
#include <vector>

#include <QDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>

#include <Core/Math/LinearAlgebra.hpp>
#include <GuiBase/Viewer/ControlPointGraphics.h>

namespace Ui {
class KeyFramedValueViewer;
} // namespace Ui

struct KeyFramedValue {
    std::string m_name;
    Ra::Core::Vector2 m_bounds;
    std::map<uint, Scalar> m_keyFrames;
};

class KeyFramedValueViewer : public QWidget {
    Q_OBJECT
  public:
    explicit KeyFramedValueViewer( const KeyFramedValue& kfv, int maxFrames = 0,
                                   QWidget* parent = 0 );
    ~KeyFramedValueViewer();

    const KeyFramedValue& getKeyFramedValue() const;
    void setKeyFramedValue( const KeyFramedValue& kfv ); // todo: deal with new max number of frames
    void setCurrentFrame( int frame );
    void setMaxFrame( int maxFrame ); // todo

  signals:
    void valueEdited();
    void saveValue();
    void loadValue();

  private:
    void setupKeyFrames();
    void updateCurve();
    void updateGrid();
    Scalar value_from_graphics( Scalar y );
    Scalar value_to_graphics( Scalar value );
    Scalar frame_to_graphics( int frame );
    Scalar getValueFromFrame( int frame );

  private slots:
    void on_m_load_pb_clicked();
    void on_m_save_pb_clicked();
    void on_m_reset_pb_clicked();

    void on_pointMoved( int i, QPointF pos );

    void on_m_minBound_dsb_valueChanged( double arg1 );
    void on_m_maxBound_dsb_valueChanged( double arg1 );
    void on_m_currentValue_dsb_valueChanged( double arg1 );

  private:
    int m_currentFrame;
    int m_currentEditFrame;
    int m_maxFrames;
    KeyFramedValue m_value;
    KeyFramedValue m_valueSave;

    Ui::KeyFramedValueViewer* m_ui;
    QGraphicsScene* m_scene;
    std::vector<ControlPointGraphics*> m_points;
    std::vector<QGraphicsLineItem*> m_curve;
    std::vector<QGraphicsLineItem*> m_grid;
};

#endif // KEYFRAMEDVALUEVIEWER_H
