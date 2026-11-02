package com.somnia.app.alarm

import android.Manifest
import android.app.AlarmManager
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.somnia.app.R
import com.somnia.app.SomniaApp
import com.somnia.app.data.Settings
import com.somnia.app.ui.MainActivity
import java.util.Calendar

object AlarmScheduler {
    const val ALARM_NOTIF = 77
    private const val REMINDER_NOTIF = 78

    private fun alarmIntent(context: Context) = PendingIntent.getBroadcast(
        context, 10, Intent(context, AlarmReceiver::class.java),
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
    )

    /** Heure limite du réveil (la sonnerie peut partir plus tôt en sommeil léger). */
    fun scheduleAlarm(context: Context, at: Long) {
        val am = context.getSystemService(AlarmManager::class.java)
        val show = PendingIntent.getActivity(context, 11, Intent(context, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE)
        val canExact = Build.VERSION.SDK_INT < 31 || am.canScheduleExactAlarms()
        if (canExact) am.setAlarmClock(AlarmManager.AlarmClockInfo(at, show), alarmIntent(context))
        else am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, at, alarmIntent(context))
    }

    fun cancelAlarm(context: Context) {
        context.getSystemService(AlarmManager::class.java).cancel(alarmIntent(context))
    }

    fun snooze(context: Context, minutes: Int = 9) {
        stopRinging(context)
        scheduleAlarm(context, System.currentTimeMillis() + minutes * 60_000L)
    }

    /**
     * Fait sonner le réveil : notification plein écran « insistante » sur le
     * canal Réveil (le son boucle jusqu'à ce qu'on l'arrête).
     */
    fun ringNow(context: Context) {
        val full = PendingIntent.getActivity(
            context, 12, Intent(context, AlarmActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
        val n = NotificationCompat.Builder(context, SomniaApp.CH_ALARM)
            .setSmallIcon(R.drawable.ic_moon)
            .setContentTitle("Bonjour ☀️")
            .setContentText("C'est l'heure de se lever")
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .setPriority(NotificationCompat.PRIORITY_MAX)
            .setFullScreenIntent(full, true)
            .setContentIntent(full)
            .setOngoing(true)
            .build()
        n.flags = n.flags or android.app.Notification.FLAG_INSISTENT
        if (Build.VERSION.SDK_INT < 33 ||
            ContextCompat.checkSelfPermission(context, Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED
        ) {
            context.getSystemService(NotificationManager::class.java).notify(ALARM_NOTIF, n)
        }
        runCatching { context.startActivity(Intent(context, AlarmActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) }
    }

    fun stopRinging(context: Context) {
        context.getSystemService(NotificationManager::class.java).cancel(ALARM_NOTIF)
    }

    // ---- Rappel du coucher ----
    private fun reminderIntent(context: Context) = PendingIntent.getBroadcast(
        context, 20, Intent(context, BedtimeReminderReceiver::class.java),
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
    )

    fun scheduleBedtime(context: Context, s: Settings) {
        val am = context.getSystemService(AlarmManager::class.java)
        am.cancel(reminderIntent(context))
        if (!s.bedtimeReminderEnabled) return
        val cal = Calendar.getInstance().apply {
            set(Calendar.HOUR_OF_DAY, s.bedtimeHour); set(Calendar.MINUTE, s.bedtimeMinute)
            set(Calendar.SECOND, 0); set(Calendar.MILLISECOND, 0)
            if (timeInMillis <= System.currentTimeMillis()) add(Calendar.DAY_OF_YEAR, 1)
        }
        am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, cal.timeInMillis, reminderIntent(context))
    }

    fun showBedtimeReminder(context: Context) {
        if (Build.VERSION.SDK_INT >= 33 &&
            ContextCompat.checkSelfPermission(context, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) return
        val open = PendingIntent.getActivity(context, 21, Intent(context, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE)
        val n = NotificationCompat.Builder(context, SomniaApp.CH_REMINDER)
            .setSmallIcon(R.drawable.ic_moon)
            .setContentTitle("Bientôt l'heure de dormir 🌙")
            .setContentText("Baissez la lumière, posez les écrans et lancez le suivi de la nuit.")
            .setContentIntent(open)
            .setAutoCancel(true)
            .build()
        context.getSystemService(NotificationManager::class.java).notify(REMINDER_NOTIF, n)
    }
}
