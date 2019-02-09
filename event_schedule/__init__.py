import ugfx, badge, gc, wifi, utime, appglue, version
from machine import Timer
import urequests as requests
from event_schedule import event_alarm
from event_schedule import fahrplan # TODO: integrate this more...

# GLOBALS
ledTimer = Timer(-1)
position = 0
schedule = {}
schedule_day = {}
schedule_event = {}

# DRAW
def draw_msg(title, desc):
    ugfx.clear(ugfx.WHITE)
    ugfx.string(0, 0, title, "PermanentMarker22", ugfx.BLACK)
    ugfx.string(0, 25, desc, "Roboto_Regular12", ugfx.BLACK)
    ugfx.set_lut(ugfx.LUT_FASTER)
    ugfx.flush()
    
def draw_alarm(desc,room):
    ugfx.clear(ugfx.WHITE)
    ugfx.string(0, 0, "Event starts soon!", "PermanentMarker22", ugfx.BLACK)
    ugfx.string(0, 25, desc, "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, 25+13, room, "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, ugfx.height()-13, "[ B: Close ]", "Roboto_Regular12", ugfx.BLACK)
    ugfx.set_lut(ugfx.LUT_FASTER)
    ugfx.flush()
    
# BACK TO HOME
def start_home(pushed):
    global ledTimer
    if(pushed):
        ledTimer.deinit()
        appglue.home()

# ALARM
def pop_alarm():
    event_alarm.alarms_read()
    current_datetime = utime.time()
    amount_of_alarms = len(event_alarm.alarms)
    for i in range(0, amount_of_alarms):
        alarm = event_alarm.alarms[i]
        c = int(alarm['timestamp']) - 10*60
        if (c < current_datetime):
            print("Deleted alarm "+str(i))
            event_alarm.alarms.pop(i)
            break
    event_alarm.alarms_write()
  
def alarm_dismiss(pressed):
    if (pressed):
        draw_msg("Please wait...", "") 
        pop_alarm()        
        appglue.start_app("event_schedule")

def alarm_check_and_notify():
    event_alarm.alarms_read()
    current_datetime = utime.time()
    
    amount_of_alarms = len(event_alarm.alarms)
    
    for i in range(0, amount_of_alarms):
        alarm = event_alarm.alarms[i]
        c = int(alarm['timestamp']) - 10*60 #10 minutes before start
        if (c < current_datetime):
            ledTimer.deinit() #Stop the background timer
            ledTimer.init(period=50, mode=Timer.PERIODIC, callback=alarmTimer_callback)
            global alarm_check_title
            print("Alarm! ("+alarm['title']+", "+str(c)+")")
            draw_alarm("Title: "+alarm['title'],"Location: "+alarm['room'])            
            ugfx.input_attach(ugfx.JOY_UP, nothing)
            ugfx.input_attach(ugfx.JOY_DOWN, nothing)
            ugfx.input_attach(ugfx.JOY_LEFT, nothing)
            ugfx.input_attach(ugfx.JOY_RIGHT, nothing)
            ugfx.input_attach(ugfx.BTN_SELECT, nothing)
            ugfx.input_attach(ugfx.BTN_A, nothing)
            ugfx.input_attach(ugfx.BTN_B, alarm_dismiss )
            
            badge.vibrator_activate(0xFF)     
            diff = c - current_datetime
            print("[EVSCH] Alarm time reached for "+alarm['title']+" ("+str(diff)+")")
            return True #Program should stop
        else:
            countdown = abs(c - current_datetime)
            print("[EVSCH] Alarm over "+str(countdown)+"s: "+alarm['title'])
    return False #Normal operation may continue
    
def alarmTimer_callback(tmr):
    global position
    position = position + 1
    if (position>255):
        badge.vibrator_activate(0xF0)
        position = 0
    ledVal = 0
    if (position==0) or (position==20) or (position==40) or (position==60) or (position==80):
        ledVal = 128
    badge.leds_send_data(bytes([0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal,0,ledVal]),24)
     
# EVENT TIMER
     
def ledTimer_callback(tmr):
    global position
    position = position + 2
    ledVal = 0
    if position>10:
        ledVal = 1
        position = 0
    badge.leds_send_data(bytes([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,ledVal,0,0,0]),24)
    
    gc.collect()
    print("Memory available: "+str(gc.mem_free()))
    
    alarm_check_and_notify()

# GENERIC INPUT EVENT

def nothing(pressed):
    ugfx.flush()
    pass

# SCREEN: EVENT

def screen_event_submit(pressed):
    if (pressed):
        print("> event submit (create alarm)")
        global schedule_event
        if (event_alarm.alarm_exists(schedule_event['guid'])):
            draw_msg("Already added", "Event already in alarm list")
        else:
            event_alarm.alarms_add(schedule_event)
            event_alarm.alarms_write()
            draw_msg("You will be notified!", "Event added to alarms")
        utime.sleep(1)
        screen_event()
        
def screen_event_back(pressed):
    if (pressed):
        screen_eventselect()

def screen_event_flip_left(pressed):
    if (pressed):
        screen_event()
    
def screen_event_flip_right(pressed):
    if (pressed):
        screen_event_detail()

def split_and_draw(x,y,text,max_lines,skip_lines):    
    words = text.split()
    line = ""
    lc = 0
    tl = 0
    for i in range(0,len(words)):
        if len(words[i])>=50:
            print("What the?!?! This word is "+str(len(words[i]))+" characters long: "+words[i])
            tl += 2
            if skip_lines>0:
                skip_lines -= 1
            elif lc<max_lines:
                ugfx.string(x, y, line, "Roboto_Regular12", ugfx.BLACK)
                y += 13
                lc += 1
            if skip_lines>0:
                skip_lines -= 1
            elif lc<max_lines:
                ugfx.string(x, y, words[i], "Roboto_Regular12", ugfx.BLACK)
                y += 13
                lc += 1
            line = ""
        elif len(line+' '+words[i])<50:
            line += ' '+words[i]
        else:
            tl += 1
            if skip_lines>0:
                skip_lines -= 1
            elif lc<max_lines:
                ugfx.string(x, y, line, "Roboto_Regular12", ugfx.BLACK)
                y += 13
                lc += 1
            line = words[i]
    tl += 1
    if skip_lines>0:
        skip_lines -= 1
    elif lc<max_lines:
        ugfx.string(x, y, line, "Roboto_Regular12", ugfx.BLACK)
    return tl

def screen_event_scroll():
    ugfx.area(0,34,ugfx.width(),ugfx.height()-15-34,ugfx.WHITE)
    global scrollPos
    global schedule_event
    split_and_draw(0,35,schedule_event['abstract'],6,scrollPos)
    ugfx.flush()

def screen_event_scroll_up(pressed):
    if pressed:
        global scrollPos
        if scrollPos>0:
            scrollPos -= 1
            screen_event_scroll()
    
def screen_event_scroll_down(pressed):
    if pressed:
        global scrollPos
        global scrollMax
        if scrollPos<scrollMax-6:
            scrollPos += 1
            screen_event_scroll()

def screen_event():
    print("> Screen event")
    global schedule_event
    ugfx.clear(ugfx.WHITE)
    ugfx.string(0, 0, schedule_event['title'], "Roboto_Regular18", ugfx.BLACK)
    ugfx.string(0, 20, schedule_event['subtitle'], "Roboto_Regular12", ugfx.BLACK)
    ugfx.line(0,33,295,33, ugfx.BLACK)
        
    global scrollPos
    scrollPos = 0
    global scrollMax
    scrollMax = split_and_draw(0,35,schedule_event['abstract'],6,0)
        
    ugfx.line(0,ugfx.height()-14,295,ugfx.height()-14, ugfx.BLACK)
    ugfx.string(0, ugfx.height()-12, "A: Add alarm | B: Back | START: Home | >: Details", "Roboto_Regular12", ugfx.BLACK)
    
    ugfx.input_attach(ugfx.JOY_UP, screen_event_scroll_up)
    ugfx.input_attach(ugfx.JOY_DOWN, screen_event_scroll_down)
    ugfx.input_attach(ugfx.JOY_LEFT, screen_event_flip_left)
    ugfx.input_attach(ugfx.JOY_RIGHT, screen_event_flip_right)
    ugfx.input_attach(ugfx.BTN_SELECT, nothing)
    ugfx.input_attach(ugfx.BTN_A, screen_event_submit)
    ugfx.input_attach(ugfx.BTN_B, screen_event_back )
    ugfx.flush()
    
def screen_event_detail():
    print("> Screen event detail")
    global schedule_event
    ugfx.clear(ugfx.WHITE)
    ugfx.string(0, 0, schedule_event['title'], "Roboto_Regular18", ugfx.BLACK)
    ugfx.string(0, 20, schedule_event['subtitle'], "Roboto_Regular12", ugfx.BLACK)
    ugfx.line(0,33,295,33, ugfx.BLACK)    
    ugfx.string(0, 35, "Type: "+schedule_event['type'], "Roboto_Regular12", ugfx.BLACK)
    if (schedule_event['do_not_record']):
        ugfx.string(0, 35+13, "Recording is not allowed!", "Roboto_Regular12", ugfx.BLACK)
    else:
        ugfx.string(0, 35+13, "Recording is allowed ("+schedule_event['recording_license']+").", "Roboto_Regular12", ugfx.BLACK)
        
    ugfx.string(0, 35+13*2, "Language: "+schedule_event['language'], "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, 35+13*3, "Room: "+schedule_event['room'], "Roboto_Regular12", ugfx.BLACK)

    #Timezone correction
    localtimestamp = schedule_event['timestamp'] #No need, fixed in firmware now.
    
    [year, month, day, hour, minute, second, weekday, yearday] = utime.localtime(localtimestamp)
        
    when = "%d-%d-%d %d:%d"% (day, month, year, hour, minute)
        
    ugfx.string(0, 35+13*4, "When: "+when, "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, 35+13*5, "Duration: "+schedule_event['duration'], "Roboto_Regular12", ugfx.BLACK)
    
    ugfx.line(0,ugfx.height()-14,295,ugfx.height()-14, ugfx.BLACK)
    ugfx.string(0, ugfx.height()-12, "A: Add alarm | B: Back | START: Home | <: Abstract", "Roboto_Regular12", ugfx.BLACK)
    
    ugfx.input_attach(ugfx.JOY_UP, nothing)
    ugfx.input_attach(ugfx.JOY_DOWN, nothing)
    ugfx.input_attach(ugfx.JOY_LEFT, screen_event_flip_left)
    ugfx.input_attach(ugfx.JOY_RIGHT, screen_event_flip_right)
    ugfx.input_attach(ugfx.BTN_SELECT, nothing)
    ugfx.input_attach(ugfx.BTN_A, screen_event_submit)
    ugfx.input_attach(ugfx.BTN_B, screen_event_back )
    ugfx.flush()

def load_event(guid):
    draw_msg("Loading...", "Downloading JSON...")
    gc.collect()
    try:
        data = requests.get("http://badge.team/schedule/event/"+guid+".json")
    except:
        draw_msg("Error", "Could not download JSON!")
        utime.sleep(5)
        screen_eventselect()
        return
    try:
        global schedule_event
        schedule_event = data.json()
    except:
        data.close()
        draw_msg("Error", "Could not decode JSON!")
        utime.sleep(5)
        screen_eventselect()
        return
    
    data.close()
    
    draw_msg("Loading...", "Rendering list...")
    screen_event()
    
# SCREEN: EVENTSELECT (DAY VIEW)

def screen_eventselect_submit(pressed):
    if (pressed):
        global options
        option_id = options.selected_index()
        options.destroy()
        global schedule_day
        event = schedule_day[option_id]
        print("> load event: "+event['title']+" ("+event['guid']+")")
        load_event(event['guid'])
        
def screen_eventselect_back(pressed):
    if (pressed):
        options.destroy()
        screen_dayselect()

def screen_eventselect():
    print("> Screen eventselect")
    global options
    global schedule
    ugfx.clear(ugfx.WHITE)
    title = "Select an event"
    ugfx.string(0, 0, title, "Roboto_Regular12", ugfx.BLACK)
    options = ugfx.List(0,15,ugfx.width(),ugfx.height())
    for event in range(0, len(schedule_day)):
        #print("Adding event "+str(event))
        evtitle = schedule_day[event]['title']
        if (len(evtitle)>50):
            evtitle = evtitle[0:47]+"..."
        options.add_item("%s" % evtitle)
    ugfx.input_attach(ugfx.JOY_UP, nothing)
    ugfx.input_attach(ugfx.JOY_DOWN, nothing)
    ugfx.input_attach(ugfx.JOY_LEFT, nothing)
    ugfx.input_attach(ugfx.JOY_RIGHT, nothing)
    ugfx.input_attach(ugfx.BTN_SELECT, nothing)
    ugfx.input_attach(ugfx.BTN_A, screen_eventselect_submit)
    ugfx.input_attach(ugfx.BTN_B, screen_eventselect_back )
    ugfx.flush()

def load_day(day):
    draw_msg("Loading...", "Downloading JSON...")
    gc.collect()
    try:
        data = requests.get("http://badge.team/schedule/day/"+str(day)+".json")
    except:
        draw_msg("Error", "Could not download JSON!")
        utime.sleep(5)
        screen_dayselect()
        return
    try:
        global schedule_day
        schedule_day = data.json()
    except:
        data.close()
        draw_msg("Error", "Could not decode JSON!")
        utime.sleep(5)
        screen_dayselect()
        return
    
    data.close()
    
    screen_eventselect()
    
# SCREEN: DAY SELECT

def screen_dayselect_submit(pressed):
    if (pressed):
        global options
        day = options.selected_index()
        options.destroy()
        print("> load day: "+str(day))
        load_day(day)
        
def fahrplan_callback(action):
    if (action==0):
        screen_dayselect()
    elif (action==1):
        start_home(True)
    else:
        print("Fahrplan error unknown action: "+str(action))
        
def load_fahrplan(day):
    draw_msg("Loading...", "Downloading JSON...")
    gc.collect()
    try:
        data = requests.get("http://badge.team/schedule/fahrplan/"+str(day)+".json")        
    except:
        draw_msg("Error", "Could not download JSON!")
        utime.sleep(5)
        screen_dayselect()
        return
    try:
        global schedule_day
        schedule_day = data.json()
    except:
        data.close()
        draw_msg("Error", "Could not decode JSON!")
        utime.sleep(5)
        screen_dayselect()
        return
    
    data.close()
    fahrplan.schedule = schedule_day
    fahrplan.callback = fahrplan_callback
    fahrplan.run()
    
def screen_dayselect_fahrplan(pressed):
    if (pressed):
        global options
        day = options.selected_index()
        options.destroy()
        print("> load fahrplan: "+str(day))
        load_fahrplan(day)
          
def screen_dayselect_back(pressed):
    if (pressed):
        options.destroy()
        screen_home()

def screen_dayselect():
    print("> Screen dayselect")
    global options
    global schedule
    ugfx.clear(ugfx.WHITE)
    title = schedule["title"]+" [ "+schedule["version"]+" ]"
    ugfx.string(0, 0, title, "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, ugfx.height()-13, "A: List | SELECT: Overview | B: Back | START: Home", "Roboto_Regular12", ugfx.BLACK)
    options = ugfx.List(0,15,ugfx.width(),ugfx.height()-29)
    try:
      for day in range(0, len(schedule['days'])):
          print("[O] Adding day "+str(day))
          options.add_item("Day %d of %d: %s"% (day+1, len(schedule['days']), schedule["days"][str(day)]))
    except:
      for day in range(0, len(schedule['days'])):
          print("[N] Adding day "+str(day))
          options.add_item("Day %d of %d: %s"% (day+1, len(schedule['days']), schedule["days"][str(day+1)]))
    ugfx.input_attach(ugfx.JOY_UP, nothing)
    ugfx.input_attach(ugfx.JOY_DOWN, nothing)
    ugfx.input_attach(ugfx.JOY_LEFT, nothing)
    ugfx.input_attach(ugfx.JOY_RIGHT, nothing)
    ugfx.input_attach(ugfx.BTN_SELECT, screen_dayselect_fahrplan)
    ugfx.input_attach(ugfx.BTN_A, screen_dayselect_submit)
    ugfx.input_attach(ugfx.BTN_B, screen_dayselect_back )
    ugfx.flush()
    
def load_daylist():
    draw_msg("Loading...", "Downloading JSON...")
    gc.collect()
    try:
        data = requests.get("http://badge.team/schedule/schedule.json")
    except:
        draw_msg("Error", "Could not download JSON!")
        utime.sleep(5)
        start_home(True)
        
    try:
        global schedule
        schedule = data.json()
    except:
        data.close()
        draw_msg("Error", "Could not decode JSON!")
        utime.sleep(5)
        start_home(True)
        
    data.close()
            
    draw_msg("Loading...", "Rendering list...")
    screen_dayselect()
    
# SCREEN: ALARM

def screen_alarm_submit(pressed):
    if (pressed):
        global options
        id = options.selected_index()
        options.destroy()
        print("> delete alarm: "+str(id))
        event_alarm.alarms_remove(id)
        event_alarm.alarms_write()
        draw_msg("Alarm removed", "You will no longer be notified")
        utime.sleep(1)
        screen_alarms()
    
def screen_alarms():
    print("> Screen alarms")
    global options
    global schedule
    ugfx.clear(ugfx.WHITE)
    title = "Alarms"
    ugfx.string(0, 0, title, "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, ugfx.height()-13, "A: Delete | B: Back | START: Home", "Roboto_Regular12", ugfx.BLACK)
    options = ugfx.List(0,15,ugfx.width(),ugfx.height()-29)
    for alarm in event_alarm.alarms:
      options.add_item(alarm['title'])
    ugfx.input_attach(ugfx.JOY_UP, nothing)
    ugfx.input_attach(ugfx.JOY_DOWN, nothing)
    ugfx.input_attach(ugfx.JOY_LEFT, nothing)
    ugfx.input_attach(ugfx.JOY_RIGHT, nothing)
    ugfx.input_attach(ugfx.BTN_SELECT, nothing)
    if len(event_alarm.alarms)<1:
        ugfx.input_attach(ugfx.BTN_A, nothing)
    else:
        ugfx.input_attach(ugfx.BTN_A, screen_alarm_submit)
    ugfx.input_attach(ugfx.BTN_B, screen_dayselect_back )
    ugfx.flush()
        
# SCREEN: HOME
    
def screen_home_submit(pressed):
    if pressed:
        global options
        option = options.selected_index()
        options.destroy()
        if (option==0):
            load_daylist()
        if (option==1):
            screen_alarms()
        if (option==2):
            draw_msg("About","")
            utime.sleep(2)
            draw_msg("About","Thank you for using our app!")
            utime.sleep(2)
            screen_home()
      
def screen_about_back(pressed):
    if pressed:
        screen_home()
    
def try_destroy_options():
    try:
        global options
        options
        options.destroy()
    except:
        pass
    
def screen_about(pressed):
    if pressed:
        try_destroy_options()
        print("> Screen about")
        global schedule
        ugfx.clear(ugfx.WHITE)
        ugfx.string(0, 0, "Event schedule", "Roboto_Regular12", ugfx.BLACK)
        ugfx.string(0, 13, "This app has been brought to you by", "Roboto_Regular12", ugfx.BLACK)
        ugfx.string(0, 13*2, "Renze Nicolai and f0x", "Roboto_Regular12", ugfx.BLACK)
        ugfx.string(0, 13*4, "(c) 2017 - GPLv3", "Roboto_Regular12", ugfx.BLACK)
        ugfx.string(0, ugfx.height()-13, "B: Back | START: Home", "Roboto_Regular12", ugfx.BLACK)
        ugfx.flush()
        ugfx.input_attach(ugfx.JOY_UP, nothing)
        ugfx.input_attach(ugfx.JOY_DOWN, nothing)
        ugfx.input_attach(ugfx.JOY_LEFT, nothing)
        ugfx.input_attach(ugfx.JOY_RIGHT, nothing)
        ugfx.input_attach(ugfx.BTN_SELECT, nothing)
        ugfx.input_attach(ugfx.BTN_A, nothing)
        ugfx.input_attach(ugfx.BTN_B, screen_about_back)
    
def screen_home():
    print("> Screen home")
    global options
    global schedule
    ugfx.clear(ugfx.WHITE)
    title = "Main menu"
    ugfx.string(0, 0, title, "Roboto_Regular12", ugfx.BLACK)
    ugfx.string(0, ugfx.height()-13, "A: Select | START: Home | SELECT: About", "Roboto_Regular12", ugfx.BLACK)
    options = ugfx.List(0,15,ugfx.width(),ugfx.height()-29)
    options.add_item("Browse schedule")
    options.add_item("Manage scheduled alarms")
    #options.add_item("About") #About the app
    ugfx.input_attach(ugfx.JOY_UP, nothing)
    ugfx.input_attach(ugfx.JOY_DOWN, nothing)
    ugfx.input_attach(ugfx.JOY_LEFT, nothing)
    ugfx.input_attach(ugfx.JOY_RIGHT, nothing)
    ugfx.input_attach(ugfx.BTN_SELECT, screen_about)
    ugfx.input_attach(ugfx.BTN_A, screen_home_submit)
    ugfx.input_attach(ugfx.BTN_B, nothing )
    ugfx.flush()
    
# MAIN PROGRAM

def program_main():  
    ugfx.init()
    ugfx.input_init()
    if version.build <= 10:
        draw_msg("Warning", "Update your firmware or times will be wrong!")
        utime.sleep(5)
    else:
	    draw_msg("Welcome!","Starting app...")
    
    if alarm_check_and_notify():
        return #Stop running normal app if alarm has triggered
    
    ugfx.input_attach(ugfx.BTN_START, start_home)
    global ledTimer
    ledTimer.init(period=1000, mode=Timer.PERIODIC, callback=ledTimer_callback)
    badge.leds_enable()
    
    draw_msg("Loading...", "WiFi: starting radio...")
    wifi.init()
        
    ssid = badge.nvs_get_str('badge', 'wifi.ssid', 'SHA2017-insecure')
    draw_msg("Loading...", "WiFi: connecting to '"+ssid+"'...")
    timeout = 100
    while not wifi.sta_if.isconnected():
        utime.sleep(0.1)
        timeout = timeout - 1
        if (timeout<1):
            draw_msg("Error", "Timeout while connecting!")
            utime.sleep(5)
            start_home(True)
            break
        else:
            pass
        
    draw_msg("Loading...","Loading alarms...")
    event_alarm.alarms_read()
    screen_home()

# Start main application
program_main()
