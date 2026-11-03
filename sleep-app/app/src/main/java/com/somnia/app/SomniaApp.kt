package com.somnia.app

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.media.AudioAttributes
import android.media.RingtoneManager
import com.somnia.app.alarm.AlarmScheduler
import com.somnia.app.data.SettingsRepository

class SomniaApp : Application() {
    override fun onCreate() {
        super.onCreate()
        com.somnia.app.ui.theme.Themes.init(this)
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel(CH_TRACKING, "Suivi du sommeil", NotificationManager.IMPORTANCE_LOW).apply {
                description = "Notification permanente pendant l'enregistrement de la nuit"
                setShowBadge(false)
            },
        )
        nm.createNotificationChannel(
            NotificationChannel(CH_ALARM, "Réveil", NotificationManager.IMPORTANCE_HIGH).apply {
                description = "Sonnerie du réveil intelligent"
                setSound(
                    RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM)
                        ?: RingtoneManager.getDefaultUri(RingtoneManager.TYPE_RINGTONE),
                    AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_ALARM)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build(),
                )
                enableVibration(true)
                vibrationPattern = longArrayOf(0, 800, 600, 800, 600)
                setBypassDnd(true)
            },
        )
        nm.createNotificationChannel(
            NotificationChannel(CH_REMINDER, "Rappel du coucher", NotificationManager.IMPORTANCE_DEFAULT),
        )
        AlarmScheduler.scheduleBedtime(this, SettingsRepository.get(this).settings.value)
    }

    companion object {
        const val CH_TRACKING = "tracking"
        const val CH_ALARM = "alarm"
        const val CH_REMINDER = "reminder"
    }
}
