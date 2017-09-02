
#include "zoomWidget.h"

#define TICK_INTERVAL			1
#define TICK_START_INDEX 		1
#define ZOOM_SLIDER_WIDTH		50
#define	ZOOM_SLIDER_HEIGHT		100

int ZoomWidget::previousLevel = -1;


/**
 * Constructor
 */
ZoomWidget::ZoomWidget(QWidget *parent)
	: QWidget(parent)
{
	/* Slider */
	slider = new QSlider;
	slider->setOrientation(Qt::Vertical);
	slider->setTickPosition(QSlider::TicksRight);
	slider->setFixedWidth(ZOOM_SLIDER_WIDTH);
	slider->setFixedHeight(ZOOM_SLIDER_HEIGHT);
	slider->setRange(TICK_START_INDEX, TICK_START_INDEX + 1);
	slider->setTickInterval(TICK_INTERVAL);
	slider->setDisabled(true);
	//slider->setInvertedAppearance(true);
	slider->setValue(slider->maximum());
	connect(slider, SIGNAL(valueChanged(int)), this, SIGNAL(sliderMoved(int)));

	/* Zoom-in button */
	zoomInButton = new QToolButton;
	zoomInButton->setIcon(QIcon(":/images/zoom_in.png"));
	zoomInButton->setText(tr("Base"));
	zoomInButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	zoomInButton->setDisabled(true);
	zoomInButton->setAutoRaise(true);
	connect(zoomInButton, SIGNAL(clicked()), this, SLOT(zoomIn()));

	/* Zoom-out button */
	zoomOutButton = new QToolButton;
	zoomOutButton->setIcon(QIcon(":/images/zoom_out.png"));
	zoomOutButton->setText(tr("Global"));
	zoomOutButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	zoomOutButton->setDisabled(true);
	zoomOutButton->setAutoRaise(true);
	connect(zoomOutButton, SIGNAL(clicked()), this, SLOT(zoomOut()));

	/* Vertical box-layout */
	vBoxLayout = new QVBoxLayout;
	//vBoxLayout->addWidget(zoomInButton, 0, Qt::AlignLeft);
	vBoxLayout->addWidget(zoomOutButton, 0, Qt::AlignLeft);
	vBoxLayout->addWidget(slider, 0, Qt::AlignLeft);
	//vBoxLayout->addWidget(zoomOutButton, 0, Qt::AlignLeft);
	vBoxLayout->addWidget(zoomInButton, 0, Qt::AlignLeft);

	/* Group box */
	groupBox = new QGroupBox(tr("Zoom"));
	groupBox->setLayout(vBoxLayout);
	groupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	QString tmp = tr("<b>Zoom</b> allows the user to zoom-in to 'Base' level or "
			"zoom-out to 'Global' level.");
	groupBox->setWhatsThis(tmp);

	contig = NULL;
}


/**
 * Destructor
 */
ZoomWidget::~ZoomWidget()
{
	delete vBoxLayout;
    delete zoomInButton;
    delete zoomOutButton;
    delete slider;
    delete groupBox;
    contig = NULL;
}


/*
 * Event handler function that is invoked when the zoom-in button
 * is clicked
 */
void ZoomWidget::zoomIn()
{
	slider->setValue(slider->value() - 1);
	previousLevel = slider->value();
}


/*
 * Event handler function that is invoked when the zoom-out button
 * is clicked
 */
void ZoomWidget::zoomOut()
{
	slider->setValue(slider->value() + 1);
	previousLevel = slider->value();
}


/**
 * Changes the slider value by the given amount
 */
void ZoomWidget::zoom(const int x)
{
	slider->setValue(slider->value() + x);
	previousLevel = slider->value();
}


/**
 * Resets zoom slider value
 *
 * If the slider position is different than the maximum value,
 * set the slider to the maximum value.
 *
 * If the slider position is same as the maximum value, then
 * just emit the sliderMoved() signal.
 */
void ZoomWidget::reset()
{
	if (slider->sliderPosition() != slider->maximum())
	{
		slider->setValue(slider->maximum());
		previousLevel = slider->value();
	}
	else
		emit sliderMoved(slider->sliderPosition());
}


/**
 * Sets the max range of the zoom slider
 */
void ZoomWidget::setIntervals(int max)
{
	slider->setRange(TICK_START_INDEX, max);
	slider->setTickInterval(TICK_INTERVAL);
}


/**
 * Enables or disables child widgets
 */
void ZoomWidget::setEnabled(bool enable)
{
	zoomInButton->setEnabled(enable);
	zoomOutButton->setEnabled(enable);
	slider->setEnabled(enable);
}


/**
 * Returns a pointer to the group box
 */
QGroupBox* ZoomWidget::getGroupBox()
{
	return groupBox;
}


/**
 * Sets the current contig
 */
void ZoomWidget::setContig(Contig *c)
{
	contig = c;
	setIntervals(contig->zoomLevels);

	/* If possible, set the zoom slider value to the previous level */
	if (previousLevel <= 0 || previousLevel > contig->zoomLevels)
		reset();
	else
		slider->setValue(previousLevel);
}

