/*
 * wpa_gui - ScanResults class
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <cstdio>

#include "scanresults.h"
#include "signalbar.h"
#include "wpagui.h"
#include "networkconfig.h"
#include "scanresultsitem.h"

#if QT_VERSION >= 0x050000
ScanResults::ScanResults(QWidget *parent, const char *, bool, Qt::WindowFlags)
	: QDialog(parent)
#else
ScanResults::ScanResults(QWidget *parent, const char *, bool, Qt::WFlags)
	: QDialog(parent)
#endif
{
	setupUi(this);

	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(scanButton, SIGNAL(clicked()), this, SLOT(scanRequest()));
	connect(scanResultsWidget,
		SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
		SLOT(bssSelected(QTreeWidgetItem *)));

	wpagui = NULL;
#if QT_VERSION >= 0x050000
	scanResultsWidget->setItemsExpandable(false);
	scanResultsWidget->setRootIsDecorated(false);
#else
	scanResultsWidget->setItemsExpandable(FALSE);
	scanResultsWidget->setRootIsDecorated(FALSE);
#endif
	scanResultsWidget->setItemDelegate(new SignalBar(scanResultsWidget));
}


ScanResults::~ScanResults()
{
}


void ScanResults::languageChange()
{
	retranslateUi(this);
}


void ScanResults::setWpaGui(WpaGui *_wpagui)
{
	wpagui = _wpagui;
	updateResults();
}


void ScanResults::updateResults()
{
	char reply[2048];
	size_t reply_len;
	int index;
	bool rs;
	int i24GNum, i5GNum; //jimmy, record 2.4G, 5g site number
	char cmd[20];

	scanResultsWidget->clear();

	index = 0;
	i24GNum =0;
	i5GNum =0;
	while (wpagui) {
		snprintf(cmd, sizeof(cmd), "BSS %d", index++);
		if (index > 1000)
			break;

		reply_len = sizeof(reply) - 1;
		if (wpagui->ctrlRequest(cmd, reply, &reply_len) < 0)
			break;
		reply[reply_len] = '\0';

		QString bss(reply);
		if (bss.isEmpty() || bss.startsWith("FAIL"))
			break;

		QString ssid, bssid, freq, signal, flags;

		QStringList lines = bss.split(QRegExp("\\n"));
		for (QStringList::Iterator it = lines.begin();
		     it != lines.end(); it++) {
			int pos = (*it).indexOf('=') + 1;
			if (pos < 1)
				continue;

			if ((*it).startsWith("bssid="))
				bssid = (*it).mid(pos);
			else if ((*it).startsWith("freq=")) {
				freq = (*it).mid(pos);
				//jimmy
				if (freq.toInt(&rs, 10) > 5000) {
					i5GNum++;
				} else {
					i24GNum++;
				}
				freq = freq + " (" + QString::number(freqToChannel(freq)) + ")";
			} else if ((*it).startsWith("level="))
				signal = (*it).mid(pos);
			else if ((*it).startsWith("flags="))
				flags = (*it).mid(pos);
            else if ((*it).startsWith("ssid=")){
                //#jimmy, TODO, unicode decode? or fix it on wpa_supplicant?
                //how to decode UTF-8-encoded string as following
                // \xe9\x9d\x92\xe8\x8a\xb1\xe7\x93\xb7\xe7\xa0\xb4\xe4\xba\x86
                // 青花瓷破了
                ssid = (*it).mid(pos);
                //QByteArray u8a = ssid.toUtf8();
                //QString ssidtmp = (*it).mid(pos);
                //ssid = QString::fromUtf8(u8a);
            }
		}

		ScanResultsItem *item = new ScanResultsItem(scanResultsWidget);
		if (item) {
			item->setText(0, ssid);
			item->setText(1, bssid);
			item->setText(2, freq);
			item->setText(3, signal);
			item->setText(4, flags);
		}

		if (bssid.isEmpty())
			break;
	}
	//jimmy
    lineEdit_BandG->setText(QString::number(i24GNum));
    lineEdit_BandA->setText(QString::number(i5GNum));
    lineEdit_Band->setText(QString::number(i5GNum+i24GNum));
}
//jimmy
//=> freq column value will convert to int to sort!!
int ScanResults::freqToChannel(QString freq)
{
	int iChannel;
	int ifreq;
	bool rs;
	ifreq = freq.toInt(&rs, 10);
	if (!rs) {
		return -1;
	}
	
	iChannel = 0;
    //TODO: 60GHz
	if (ifreq > 5000) {
		//channel= (freq - 5150)/5 + 30
		iChannel = (ifreq-5150)/5 + 30;
	} else if (ifreq > 2400) {
		//freq = (channel -1)*5 + 2412
		//channel= (freq - 2412)/5 + 1
		iChannel = (ifreq-2412)/5 + 1;
	}
	
	return iChannel;
}
void ScanResults::scanRequest()
{
	char reply[10];
	size_t reply_len = sizeof(reply);
    
	if (wpagui == NULL)
		return;
    
	wpagui->ctrlRequest("SCAN", reply, &reply_len);
}


void ScanResults::getResults()
{
	updateResults();
}


void ScanResults::bssSelected(QTreeWidgetItem *sel)
{
	NetworkConfig *nc = new NetworkConfig();
	if (nc == NULL)
		return;
	nc->setWpaGui(wpagui);
	nc->paramsFromScanResults(sel);
	nc->show();
	nc->exec();
}
