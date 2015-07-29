/*
 * OK this class forms the base of the abstraction layer,
 * it is inhereted by the serial port abstractio, inputs etc
 *
 *
*/

#include "smsabstractionclass.h"

SmsAbstractionClass::SmsAbstractionClass(QWidget *parent) : QMainWindow(parent)
{

}

void SmsAbstractionClass::generateTrigger()
{
    // debug
    emit triggerDetected(42);
}
