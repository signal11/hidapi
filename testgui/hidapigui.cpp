/*******************************************************
 Demo Program for HIDAPI
 
 Alan Ott
 Signal 11 Software

 2010-07-20

 Copyright 2010, All Rights Reserved
 
 This contents of this file may be used by anyone
 for any reason without any conditions and may be
 used as a starting point for your own applications
 which use HIDAPI.
********************************************************/


#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QGroupBox>
#include <QTimer>
#include <QDebug>

#include "hidapi.h"
//#include "mac_support.h"

static QString formatBytesAsHex(QByteArray bytes)
{
    QString s;
    for (int i = 0; i < bytes.count(); i++) {
        quint8 byte = static_cast<quint8>(bytes.at(i));
        s += QString("%1").arg((int) byte, 2, 16, QChar('0'));
        if ((i+1) % 4 == 0)
            s += " ";
        if ((i+1) % 16 == 0)
            s += "\n";
    }
    return s;
}


static QString formatDevice(struct hid_device_info *device_info)
{
    return QObject::tr("%1:%2 %3 %4").
            arg(device_info->vendor_id, 4, 16, QChar('0')).
            arg(device_info->product_id, 4, 16, QChar('0')).
            arg(QString::fromWCharArray(device_info->manufacturer_string)).
            arg(QString::fromWCharArray(device_info->product_string));
}

class MainWindow : public QMainWindow {
	Q_OBJECT
	
public:

	
private:
	QListWidget *device_list;
    QPushButton *connect_button;
    QPushButton *disconnect_button;
    QPushButton *rescan_button;
    QPushButton *output_button;
	QLabel *connected_label;
    QLineEdit *output_text;
    QLineEdit *output_len;
    QPushButton *feature_button;
    QPushButton *get_feature_button;
    QLineEdit *feature_text;
    QLineEdit *feature_len;
    QLineEdit *get_feature_text;
    QTextEdit *input_text;

	struct hid_device_info *devices;
	hid_device *connected_device;

    QByteArray getDataFromTextField(QLineEdit *tf);
    int getLengthFromTextField(QLineEdit *tf);

    QTimer timer;

public:
	MainWindow();
	~MainWindow();

    void create();
private slots:	
	long onConnect();
	long onDisconnect();
	long onRescan();
	long onSendOutputReport();
	long onSendFeatureReport();
	long onGetFeatureReport();
	long onClear();
	long onTimeout();
	long onMacTimeout();
};

// FOX 1.7 changes the timeouts to all be nanoseconds.
// Fox 1.6 had all timeouts as milliseconds.
#if (FOX_MINOR >= 7)
	const int timeout_scalar = 1000*1000;
#else
	const int timeout_scalar = 1;
#endif

MainWindow *g_main_window;

MainWindow::MainWindow()
{
	devices = NULL;
	connected_device = NULL;

    setWindowTitle(tr("HIDAPI Test Tool"));

    QWidget *vf = new QWidget(this); // top-level vertical
    QVBoxLayout* vbl = new QVBoxLayout(vf);
    setCentralWidget(vf);

    QLabel *label = new QLabel("HIDAPI Test Tool");
	label->setFont(QFont("Arial", 14, QFont::Bold));
    vbl->addWidget(label);

    vbl->addWidget(new QLabel(
        "Select a device and press Connect."));
    vbl->addWidget(new QLabel(
		"Output data bytes can be entered in the Output section, \n"
		"separated by space, comma or brackets. Data starting with 0x\n"
		"is treated as hex. Data beginning with a 0 is treated as \n"
        "octal. All other data is treated as decimal."));
    vbl->addWidget(new QLabel(
        "Data received from the device appears in the Input section."));
    vbl->addWidget(new QLabel(
		"Optionally, a report length may be specified. Extra bytes are\n"
		"padded with zeros. If no length is specified, the length is \n"
        "inferred from the data."));

	// Device List and Connect/Disconnect buttons
    QHBoxLayout* devicesHBox = new QHBoxLayout;
    vbl->addLayout(devicesHBox);
    device_list = new QListWidget;
    devicesHBox->addWidget(device_list);

    QVBoxLayout* buttonLayout = new QVBoxLayout;
    devicesHBox->addLayout(buttonLayout);
    connect_button = new QPushButton("Connect");
    buttonLayout->addWidget(connect_button);
    connect(connect_button, SIGNAL(clicked()), this, SLOT(onConnect()));

    disconnect_button = new QPushButton("Disconnect");
    disconnect_button->setEnabled(false);
    buttonLayout->addWidget(disconnect_button);
    connect(disconnect_button, SIGNAL(clicked()), this, SLOT(onDisconnect()));

    rescan_button = new QPushButton("Re-Scan devices");
    buttonLayout->addWidget(rescan_button);
    connect(rescan_button, SIGNAL(clicked()), this, SLOT(onRescan()));

    buttonLayout->addStretch(1);

    connected_label = new QLabel("Disconnected");
    vbl->addWidget(connected_label);

	// Output Group Box
    QGroupBox *gb = new QGroupBox("Output");
    vbl->addWidget(gb);
    QGridLayout* matrix = new QGridLayout(gb);

    matrix->addWidget(new QLabel("Data"), 0, 0);
    matrix->addWidget(new QLabel("Length"), 0, 1);

    output_text = new QLineEdit;
    matrix->addWidget(output_text, 1, 0);
	output_text->setText("1 0x81 0");
    output_len = new QLineEdit();
    matrix->addWidget(output_len, 1, 1);

    output_button = new QPushButton("Send Output Report");
    matrix->addWidget(output_button, 1, 2);

    connect(output_button, SIGNAL(clicked()), this, SLOT(onSendOutputReport()));
    output_button->setEnabled(false);

    feature_text = new QLineEdit();
    matrix->addWidget(feature_text, 2, 0);
    feature_len = new QLineEdit();
    matrix->addWidget(feature_len, 2, 1);

    feature_button = new QPushButton("Send Feature Report");
    feature_button->setEnabled(false);
    matrix->addWidget(feature_button, 2, 2);
    connect(feature_button, SIGNAL(clicked()), this, SLOT(onSendFeatureReport()));

    get_feature_text = new QLineEdit();
    matrix->addWidget(get_feature_text, 3, 0);

    get_feature_button = new QPushButton("Get Feature Report");
    get_feature_button->setEnabled(false);
    connect(get_feature_button, SIGNAL(clicked()), this, SLOT(onGetFeatureReport()));
    matrix->addWidget(get_feature_button, 3, 2);

    // spacer at right-most column
    matrix->addItem(new QSpacerItem(QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 3);
    matrix->setColumnStretch(3, 1);

	// Input Group Box
    gb = new QGroupBox("Input");
    QGridLayout* inputGrid = new QGridLayout(gb);
    vbl->addWidget(gb);

    input_text = new QTextEdit();
    input_text->setReadOnly(true);
    inputGrid->addWidget(input_text, 0, 0, 1, 2);

    QPushButton* clearButton = new QPushButton("Clear");
    connect(clearButton, SIGNAL(clicked()), this, SLOT(onClear()));
    inputGrid->addWidget(clearButton, 1, 1);
    inputGrid->setColumnStretch(0, 1);

    timer.setInterval(5);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

MainWindow::~MainWindow()
{
	if (connected_device)
		hid_close(connected_device);
	hid_exit();
}

void
MainWindow::create()
{
	show();

	onRescan();
	

#ifdef __APPLE__
//	init_apple_message_system();
#endif
	
	//getApp()->addTimeout(this, ID_MAC_TIMER,
	//	50 * timeout_scalar /*50ms*/);
}

long
MainWindow::onConnect()
{
	if (connected_device != NULL)
		return 1;
	
    QListWidgetItem *item = device_list->currentItem();
	if (!item)
		return -1;

    QString devicePath = item->data(Qt::UserRole).toString();
// walk enumeration to find the hid_device_info
    struct hid_device_info *dev;
    for (dev = devices; dev != NULL; dev = dev->next) {
        if (QString::fromLatin1(dev->path) == devicePath) {
            break;
        }
    } // of device enumeration walk

    if (dev == NULL) {
        QMessageBox::critical(this, "Device Error",
                              tr("Unable To Find Device %1").arg(devicePath));
        return -1;
    }

    connected_device =  hid_open_path(devicePath.toLatin1().constData());
	
	if (!connected_device) {
        QMessageBox::critical(this, "Device Error",
                              tr("Unable To Connect to Device at %1").arg(devicePath));
		return -1;
	}
	
	hid_set_nonblocking(connected_device, 1);

    timer.start();
    connected_label->setText(tr("Connected to %1").arg(formatDevice(dev)));

    output_button->setEnabled(true);
    feature_button->setEnabled(true);
    get_feature_button->setEnabled(true);
    connect_button->setEnabled(false);
    disconnect_button->setEnabled(true);
	input_text->setText("");

	return 1;
}

long
MainWindow::onDisconnect()
{
	hid_close(connected_device);
	connected_device = NULL;
	connected_label->setText("Disconnected");

    output_button->setEnabled(false);
    feature_button->setEnabled(false);
    get_feature_button->setEnabled(false);
    connect_button->setEnabled(true);
    disconnect_button->setEnabled(false);

    timer.stop();
	return 1;
}

long
MainWindow::onRescan()
{
	struct hid_device_info *cur_dev;

    device_list->clear();
	
	// List the Devices
	hid_free_enumeration(devices);
	devices = hid_enumerate(0x0, 0x0);
	cur_dev = devices;	
	while (cur_dev) {
		// Add it to the List Box.
        QString s = formatDevice(cur_dev);
        QString usage_str = tr(" (usage: %1:%2)").
                arg(cur_dev->usage_page, 2, 16, QChar('0')).
                arg(cur_dev->usage, 2, 16, QChar('0'));

		s += usage_str;
        QListWidgetItem *li = new QListWidgetItem(s);
        li->setData(Qt::UserRole, QString::fromLatin1(cur_dev->path));
        device_list->addItem(li);
		cur_dev = cur_dev->next;
	}

    if (device_list->count() == 0)
        device_list->addItem("*** No Devices Connected ***");
	else {
        device_list->setCurrentRow(0);
	}

	return 1;
}

QByteArray
MainWindow::getDataFromTextField(QLineEdit *tf)
{
    QRegExp delim("\\W");
    QString data = tf->text();
    QByteArray result;

    foreach (QString token, data.split(delim)) {
        bool ok;
        quint32 val = token.toUInt(&ok, 0);// will parse 0 and 0x prefixes
        if (!ok) {
            return QByteArray();
        }
        result.append(val);
    }

    return result;
}

/* getLengthFromTextField()
   Returns length:
	 0: empty text field
	>0: valid length
	-1: invalid length */
int
MainWindow::getLengthFromTextField(QLineEdit *tf)
{
    if (tf->text().isEmpty()) {
        return 0;
    }

    bool ok;
    int result = tf->text().toInt(&ok);
    if (!ok) return -1;
    return result;
}

long
MainWindow::onSendOutputReport()
{
    size_t len;
	int textfield_len;

	textfield_len = getLengthFromTextField(output_len);
    QByteArray data = getDataFromTextField(output_text);

	if (textfield_len < 0) {
        QMessageBox::critical(this, "Invalid length", "Length field is invalid. Please enter a number in hex, octal, or decimal.");
		return 1;
	}

    len = (textfield_len)? textfield_len: data.size();

    int res = hid_write(connected_device, (const unsigned char*) data.constData(), len);
	if (res < 0) {
        QMessageBox::critical(this, "Error Writing",
                              tr("Could not write to device. Error reported was: %1").
                              arg(QString::fromWCharArray(hid_error(connected_device))));
	}
	
	return 1;
}

long
MainWindow::onSendFeatureReport()
{
    int textfield_len = getLengthFromTextField(feature_len);
    QByteArray data = getDataFromTextField(feature_text);

	if (textfield_len < 0) {
        QMessageBox::critical(this, "Invalid length", "Length field is invalid. Please enter a number in hex, octal, or decimal.");
		return 1;
	}

    size_t len = (textfield_len)? textfield_len: data.size();

    int res = hid_send_feature_report(connected_device, (const unsigned char*) data.constData(), len);
	if (res < 0) {
        QMessageBox::critical(this, "Error Writing",
                              tr("Could not send feature report to device. Error reported was: %1").
                              arg(QString::fromWCharArray(hid_error(connected_device))));
	}

	return 1;
}

long
MainWindow::onGetFeatureReport()
{
    QByteArray data = getDataFromTextField(get_feature_text);
    if (data.size() != 1) {
        QMessageBox::critical(this, "Too many numbers", "Enter only a single report number in the text field");
        return 0;
	}

    data.resize(256);
    int res = hid_get_feature_report(connected_device, (unsigned char*) data.data(), data.size());
	if (res < 0) {
        QMessageBox::critical(this, "Error Getting Report",
                              tr("Could not get feature report from device. Error reported was: %1").
                              arg(QString::fromWCharArray(hid_error(connected_device))));
        return 0;
	}

	if (res > 0) {
        QString s = tr("Returned Feature Report. %1 bytes:\n").arg(res);
        data.resize(res);
        s += formatBytesAsHex(data) + "\n";
        input_text->setText(s);
    //	input_text->setBottomLine(INT_MAX);
	}
	
	return 1;
}

long
MainWindow::onClear()
{
	input_text->setText("");
	return 1;
}

long
MainWindow::onTimeout()
{
    QByteArray buf;
    buf.resize(256);
    int res = hid_read(connected_device, (unsigned char*) buf.data(), buf.size());
	
	if (res > 0) {
		QString s(tr("Received %1 bytes:\n").arg(res));
        buf.truncate(res);
        s += formatBytesAsHex(buf);
		s += "\n";
        input_text->append(s);
    //	input_text->setBottomLine(INT_MAX);
	}

	if (res < 0) {
        input_text->append("hid_read() returned error\n");
        //input_text->setBottomLine(INT_MAX);
	}

	return 1;
}

long
MainWindow::onMacTimeout()
{
#ifdef __APPLE__
    //check_apple_events();
	
    //getApp()->addTimeout(this, ID_MAC_TIMER,
    //	50 * timeout_scalar /*50ms*/);
#endif

	return 1;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
	g_main_window = new MainWindow();
    g_main_window->create();

	app.exec();

    delete g_main_window;
	return 0;
}

#include "hidapigui.moc"
