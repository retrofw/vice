/*
 * ui_device.cc - Device settings
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
 *  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */
 
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <Path.h>
#include <RadioButton.h>
#include <Slider.h>
#include <string.h>
#include <TabView.h>
#include <TextControl.h>
#include <Window.h>

extern "C" {
#include "attach.h"
#include "drive.h"
#include "resources.h"
#include "ui.h"
#include "ui_device.h"
#include "vsync.h"
}

static const char *resource_p00_name[] = {
    "FSDevice%dConvertP00",
    "FSDevice%dSaveP00",
    "FSDevice%dHideCBMFiles"
};

static const char *p00_text[] = {
    "Read P00 files",
    "Write P00 files",
    "Hide non-P00 files"
};


class DeviceView : public BView {
    public:
        DeviceView(BRect r, int device_num);
        void UpdateP00(int device_num);
        BTextControl *dirtextcontrol;
        BCheckBox *checkboxp00[3];
        BSlider *rpm_slider;
        BSlider *wobble_slider;
};

DeviceView::DeviceView(BRect r, int device_num) : BView(r, "device_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
    BButton *button;
    BTextView *instruction;
    BMessage *msg;
    BBox *box;
    BRect frame;
    const char *disk_image, *dir;
    int i;
    char rpm_res_name[32];
    char wobble_res_name[32];

    int rpm_value;
    int wobble_value;

    const char *instruction_text = "Remember that these settings only work if\n"
                                   "  - True Drive Emulation is disabled\n"
                                   "  - Virtual Devices are enabled\n"
                                   "  - no diskimage is attached";


    BView::SetViewColor(220, 220, 220, 0);

    disk_image = file_system_get_disk_name(device_num);
    resources_get_string_sprintf("FSDevice%dDir", &dir, device_num);
    r.InsetBy(10, 10);
    box = new BBox(r);
    AddChild(box);

    /* the directory related controls */
    msg = new BMessage(MESSAGE_DEVICE_DIRECTORY);
    msg->AddInt32("device", device_num);
    dirtextcontrol = new BTextControl(BRect(10, 10, box->Bounds().right - 10, 20), "dir text", "Directory", dir,msg);
    dirtextcontrol->SetDivider(60);
    box->AddChild(dirtextcontrol);

    msg = new BMessage(MESSAGE_DEVICE_BROWSE);
    msg->AddInt32("device", device_num);
    button = new BButton(BRect(70,30,130,40), "browsedir", "Browse", msg);
    box->AddChild(button);

    for (i = 0; i < 3; i++) {
        msg = new BMessage(MESSAGE_DEVICE_P00);
        msg->AddInt32("type", i);
        msg->AddInt32("device", device_num);

        checkboxp00[i] = new BCheckBox(BRect(150, 30 + i * 20, 300, 40 + i * 20), p00_text[i], p00_text[i], msg);
        box->AddChild(checkboxp00[i]);
    }
    UpdateP00(device_num);

    /* some explanations */
    frame = box->Bounds();
    frame.InsetBy(10, 10);
    frame.top += 90;
    frame.bottom -= 125;
    instruction = new BTextView(frame, "instructions", BRect(20, 5, frame.Width() - 20, 40), B_FOLLOW_NONE, B_WILL_DRAW);
    instruction->MakeEditable(false);
    instruction->MakeSelectable(false);
    instruction->SetViewColor(180, 180, 180, 0);
    instruction->SetText(instruction_text);
    box->AddChild(instruction);    /* Add drive RPM and Wobble items */
    sprintf(rpm_res_name, "Drive%dRPM", device_num);
    resources_get_int(rpm_res_name, &rpm_value);

    msg = new BMessage(MESSAGE_DEVICE_RPM);
    msg->AddInt32("device", device_num);
    rpm_slider = new BSlider(BRect(10, 180, 320, 30), rpm_res_name, "Drive RPM",
            msg, 28000, 32000, B_TRIANGLE_THUMB);
    rpm_slider->SetValue(rpm_value);
    rpm_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    rpm_slider->SetHashMarkCount(21);
    rpm_slider->SetLimitLabels("280.00", "320.00");
    box->AddChild(rpm_slider);

    sprintf(wobble_res_name, "Drive%dWobble", device_num);
    resources_get_int(wobble_res_name, &wobble_value);
 
    msg = new BMessage(MESSAGE_DEVICE_WOBBLE);
    msg->AddInt32("device", device_num);
    wobble_slider = new BSlider(BRect(10, 245, 320, 30), wobble_res_name,
            "Drive Wobble", msg, 0, 500, B_TRIANGLE_THUMB);
    wobble_slider->SetValue(wobble_value);
    wobble_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    wobble_slider->SetHashMarkCount(26);
    wobble_slider->SetLimitLabels("0", "5.00");
    box->AddChild(wobble_slider);

}

void DeviceView::UpdateP00(int device_num)
{
    int res_value;
    int i;

    for (i = 0; i < 3; i++) {
        resources_get_int_sprintf(resource_p00_name[i], &res_value, device_num);
        checkboxp00[i]->SetValue(res_value);
    }
}

class DeviceWindow : public BWindow {
    public:
        DeviceWindow();
        ~DeviceWindow();
        virtual void MessageReceived(BMessage *msg);
    private:
        DeviceView *dv[4]; /* pointers to the Devices 8-11 */
        BTextControl *printertextcontrol;
        BSlider *volume_slider;
};

static DeviceWindow *devicewindow = NULL;

DeviceWindow::DeviceWindow() 
    : BWindow(BRect(50, 50, 420, 500),"Device settings", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE) 
{
    BBox *box;
    BRect frame;
    BTabView *tabview;
    BTab *tab;
    BMessage *msg;
    BTextView *instruction;
    int device_num;
    char str[20];
    int volume_value;

    frame = Bounds();
    frame.bottom -= 90;
    tabview = new BTabView(frame, "tab_view");
    tabview->SetViewColor(220, 220, 220, 0);

    frame = tabview->Bounds();
    frame.InsetBy(5, 5);
    frame.OffsetTo(3, 3);
    frame.bottom -= tabview->TabHeight();
    /* the disk devices 8-11 */
    for (device_num = 8; device_num < 12; device_num++) {
        tab = new BTab();
        tabview->AddTab(dv[device_num - 8] = new DeviceView(frame, device_num), tab);
        sprintf(str, "Drive %d", device_num);
        tab->SetLabel(str);
    }

    AddChild(tabview);
    tabview->SetTabWidth(B_WIDTH_FROM_WIDEST);

    // this code sucks, but so does the BeOS 'layout' handling

    frame = Bounds();
    frame.top = 360;
    frame.InsetBy(0, 0);
    box =  new BBox(frame);
    box->SetViewColor(220, 220, 220, 0);

    // add volume slider
    resources_get_int("DriveSoundEmulationVolume", &volume_value);
    msg = new BMessage(MESSAGE_DEVICE_VOLUME);
    volume_slider = new BSlider(BRect(20, 10, 300, 30),
            "DriveSoundEmulationVolume",
            "Drive sound emulation volume",
            msg, 0, 4000, B_TRIANGLE_THUMB);
    volume_slider->SetValue(volume_value);
    volume_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    volume_slider->SetHashMarkCount(11);
    volume_slider->SetLimitLabels("0", "100");
    box->AddChild(volume_slider);
    
    AddChild(box); 

    Show();
}

DeviceWindow::~DeviceWindow() 
{
    devicewindow = NULL;
}

void DeviceWindow::MessageReceived(BMessage *msg)
{
    int32 device_num;
    int32 resource_index;
    char str[256];
    const char *s;
    int res_val;
    BFilePanel *filepanel;
    BMessage *newmsg;

    msg->FindInt32("device", &device_num);

    switch (msg->what) {
        case MESSAGE_DEVICE_P00:
            msg->FindInt32("type", &resource_index);
            resources_get_int_sprintf(resource_p00_name[resource_index], &res_val, device_num);
            resources_set_int_sprintf(resource_p00_name[resource_index], !res_val, device_num);
            /* the p00-resources have side effects on each other */
            dv[device_num - 8]->UpdateP00(device_num);
            break;
        case MESSAGE_DEVICE_BROWSE:
            resources_get_string_sprintf("FSDevice%dDir", &s, device_num);
            filepanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_DIRECTORY_NODE, false, NULL, NULL, true);
            sprintf(str,"Choose directory for device %d", device_num);
            filepanel->Window()->SetTitle(str);
            filepanel->SetPanelDirectory(s);
            /* we have to remember the device number */
            newmsg = new BMessage(MESSAGE_DEVICE_BROWSE_END);
            newmsg->AddInt32("device", device_num);
            filepanel->SetMessage(newmsg);
            delete newmsg;
            filepanel->Show();
            break;
        case MESSAGE_DEVICE_BROWSE_END:
            {
                entry_ref ref;
                BPath *path;

                msg->FindRef("refs", 0, &ref);
                path = new BPath(&ref);

                resources_set_string_sprintf("FSDevice%dDir", path->Path(), device_num);
                dv[device_num - 8]->dirtextcontrol->SetText(path->Path());
                break;
            }
        case MESSAGE_DEVICE_DIRECTORY:
            resources_set_string_sprintf("FSDevice%dDir", dv[device_num - 8]->dirtextcontrol->Text(), device_num);
            break;

        case MESSAGE_DEVICE_RPM:
            res_val = dv[device_num - 8]->rpm_slider->Value();
            resources_set_int_sprintf("Drive%dRPM", res_val, device_num);
            break;

        case MESSAGE_DEVICE_WOBBLE:
            res_val = dv[device_num - 8]->wobble_slider->Value();
            resources_set_int_sprintf("Drive%dWobble", res_val, device_num);
            break;

        case MESSAGE_DEVICE_VOLUME:
            res_val = volume_slider->Value();
            resources_set_int("DriveSoundEmulationVolume", res_val);
            break;

        default:
            BWindow::MessageReceived(msg);
    }
}

void ui_device()
{
    thread_id devicethread;
    status_t exit_value;

    if (devicewindow != NULL) {
        return;
    }

    devicewindow = new DeviceWindow;

    vsync_suspend_speed_eval();

    /* wait until window closed */
    devicethread = devicewindow->Thread();
    wait_for_thread(devicethread, &exit_value);
}
