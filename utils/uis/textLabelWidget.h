#ifndef TEXTLABELWIDGET_H
#define TEXTLABELWIDGET_H

#include <QWidget>
#include "ui_textLabelWidget.h"
#include "../viAreaWidget.h"

class TextLabelWidget : public ViAreaWidget
{
    Q_OBJECT

public:
    TextLabelWidget(QWidget *parent = 0);
    ~TextLabelWidget();

    void setText(const QString &string)
    {
    	ui.label->setText(string);
    }

private:
    Ui::TextLabelWidgetClass ui;
};

#endif // TEXTLABELWIDGET_H
