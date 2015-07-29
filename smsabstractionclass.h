#ifndef ABSTRACTIONBASECLASS_H
#define ABSTRACTIONBASECLASS_H

#include <QMainWindow>
#include <QObject>

class SmsAbstractionClass : public QMainWindow
{
    Q_OBJECT
public:
    explicit SmsAbstractionClass(QWidget *parent = 0);

signals:
    void triggerDetected(int triggerCode);

public slots:
    void generateTrigger(void);
};

#endif // ABSTRACTIONBASECLASS_H
