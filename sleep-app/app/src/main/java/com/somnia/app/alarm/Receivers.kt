package com.somnia.app.alarm

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import com.somnia.app.data.SettingsRepository

class AlarmReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) = AlarmScheduler.ringNow(context)
}

class BedtimeReminderReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        AlarmScheduler.showBedtimeReminder(context)
        AlarmScheduler.scheduleBedtime(context, SettingsRepository.get(context).settings.value)
    }
}

class BootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action == Intent.ACTION_BOOT_COMPLETED) {
            AlarmScheduler.scheduleBedtime(context, SettingsRepository.get(context).settings.value)
        }
    }
}
