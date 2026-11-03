package com.somnia.app.widget

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.widget.RemoteViews
import com.somnia.app.R
import com.somnia.app.data.SettingsRepository
import com.somnia.app.service.SleepTrackingService
import com.somnia.app.service.TrackingState
import com.somnia.app.ui.Agent
import com.somnia.app.ui.AgentLines
import com.somnia.app.ui.Fmt
import com.somnia.app.ui.MainActivity
import java.util.Calendar

/**
 * Widget de l'écran d'accueil : l'agent, sa phrase selon l'heure et le bouton
 * « Commencer la nuit » (ou « Je me réveille » pendant la nuit).
 */
class DodoWidget : AppWidgetProvider() {
    override fun onUpdate(context: Context, manager: AppWidgetManager, ids: IntArray) {
        ids.forEach { manager.updateAppWidget(it, views(context)) }
    }

    companion object {
        const val ACTION_START_NIGHT = "com.somnia.app.START_NIGHT"

        fun refresh(context: Context) {
            val mgr = AppWidgetManager.getInstance(context)
            val ids = mgr.getAppWidgetIds(ComponentName(context, DodoWidget::class.java))
            if (ids.isNotEmpty()) ids.forEach { mgr.updateAppWidget(it, views(context)) }
        }

        private fun views(context: Context): RemoteViews {
            val live = TrackingState.state.value
            val agent = SettingsRepository.get(context).settings.value.agent
            val hour = Calendar.getInstance().get(Calendar.HOUR_OF_DAY)
            val v = RemoteViews(context.packageName, R.layout.widget_dodo)
            val image = when {
                live != null && agent == 1 -> R.drawable.night_agent2_f3
                live != null -> R.drawable.night_agent1_f3
                agent == 1 -> Agent.awakeDrawable(hour)
                else -> R.drawable.agent_awake
            }
            v.setImageViewResource(R.id.widget_agent, image)
            v.setTextViewText(
                R.id.widget_line,
                if (live != null) "Chut… je surveille votre nuit depuis ${Fmt.time(live.startTime)}." else AgentLines.forHour(hour),
            )
            v.setTextViewText(R.id.widget_button, if (live != null) "Je me réveille" else "Commencer la nuit")
            val action = if (live != null) {
                PendingIntent.getService(
                    context, 31, Intent(context, SleepTrackingService::class.java).setAction(SleepTrackingService.ACTION_STOP),
                    PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
                )
            } else {
                // On passe par l'appli (au premier plan) : Android l'exige pour utiliser le micro.
                PendingIntent.getActivity(
                    context, 32, Intent(context, MainActivity::class.java).setAction(ACTION_START_NIGHT)
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP),
                    PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
                )
            }
            v.setOnClickPendingIntent(R.id.widget_button, action)
            v.setOnClickPendingIntent(
                R.id.widget_agent,
                PendingIntent.getActivity(context, 33, Intent(context, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE),
            )
            return v
        }
    }
}
