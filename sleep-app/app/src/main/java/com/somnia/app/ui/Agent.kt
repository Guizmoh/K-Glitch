package com.somnia.app.ui

import android.content.ComponentName
import android.content.Context
import android.content.pm.PackageManager
import androidx.compose.runtime.compositionLocalOf
import com.somnia.app.data.SettingsRepository

/** Personnage affiché : 0 = l'agent moustachu, 1 = le gros agent au nez rouge. */
val LocalAgent = compositionLocalOf { 0 }

/** Heure utilisée pour l'humeur de l'agent (null = heure actuelle). */
val LocalHour = compositionLocalOf<Int?> { null }

/** Action quand on touche la tête du policier (null = pas cliquable). */
val LocalAgentToggle = compositionLocalOf<(() -> Unit)?> { null }

object Agent {
    const val COUNT = 2

    /** Humeur du gros agent selon l'heure : sourire de 8 h à minuit, neutre de minuit à 1 h, en colère ensuite. */
    fun awakeDrawable(hour: Int): Int = when (hour) {
        in 8..23 -> com.somnia.app.R.drawable.agent2_smile
        0 -> com.somnia.app.R.drawable.agent2_neutral
        else -> com.somnia.app.R.drawable.agent2_angry
    }

    /** Animation de la bave : images 4, 5, 6, 6, 5, 4 en boucle. */
    val SLEEP_SEQUENCE = intArrayOf(
        com.somnia.app.R.drawable.agent2_sleep1, com.somnia.app.R.drawable.agent2_sleep2, com.somnia.app.R.drawable.agent2_sleep3,
        com.somnia.app.R.drawable.agent2_sleep3, com.somnia.app.R.drawable.agent2_sleep2, com.somnia.app.R.drawable.agent2_sleep1,
    )

    /** Change d'icône sur le téléphone (séparé pour pouvoir le tester). */
    fun interface IconSwitcher { fun enableOnly(index: Int) }

    private fun systemSwitcher(context: Context) = IconSwitcher { agent ->
        val pm = context.packageManager
        for (i in 0 until COUNT) {
            val state = if (i == agent) PackageManager.COMPONENT_ENABLED_STATE_ENABLED else PackageManager.COMPONENT_ENABLED_STATE_DISABLED
            runCatching {
                pm.setComponentEnabledSetting(
                    ComponentName(context.packageName, "com.somnia.app.AgentIcon${i + 1}"), state, PackageManager.DONT_KILL_APP,
                )
            }
        }
    }

    /** Icône à appliquer quand l'appli passera en arrière-plan (null = rien à faire). */
    @Volatile var pendingIcon: Int? = null
        private set

    /**
     * Change de policier dans l'appli tout de suite. L'icône du téléphone n'est changée
     * qu'en quittant l'appli : la changer pendant qu'on l'utilise fait fermer l'écran
     * par Android (retour à l'accueil du téléphone).
     */
    fun set(context: Context, agent: Int) {
        SettingsRepository.get(context).update { it.copy(agent = agent) }
        com.somnia.app.widget.DodoWidget.refresh(context)
        // L'icône du téléphone suit le policier choisi (version qui rigole), en quittant l'appli
        requestIcon(agent)
    }

    /** Mémorise l'icône voulue, sans toucher au téléphone. */
    fun requestIcon(agent: Int) { pendingIcon = agent }

    /** Appelé quand l'appli n'est plus à l'écran. */
    fun applyPendingIcon(context: Context, switcher: IconSwitcher = systemSwitcher(context)) {
        val icon = pendingIcon ?: return
        pendingIcon = null
        switcher.enableOnly(icon)
    }

    fun next(context: Context) = set(context, (SettingsRepository.get(context).settings.value.agent + 1) % COUNT)
}
