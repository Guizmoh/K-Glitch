package com.somnia.app.alarm

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import com.somnia.app.data.SettingsRepository
import kotlinx.coroutines.launch

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
            AlarmScheduler.scheduleWeekly(context)
        }
    }
}

/** Lundi matin : notification du bilan de la semaine. */
class WeeklyReportReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val pending = goAsync()
        kotlinx.coroutines.CoroutineScope(kotlinx.coroutines.Dispatchers.IO).launch {
            try {
                val sessions = com.somnia.app.data.SomniaDatabase.get(context).dao().finishedSessions()
                com.somnia.app.analysis.WeeklyReport.compute(sessions, System.currentTimeMillis())?.let {
                    AlarmScheduler.showWeekly(context, it.summary())
                }
                AlarmScheduler.scheduleWeekly(context)
            } finally { pending.finish() }
        }
    }
}
