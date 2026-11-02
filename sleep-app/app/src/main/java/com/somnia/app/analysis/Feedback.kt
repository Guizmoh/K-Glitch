package com.somnia.app.analysis

import com.somnia.app.audio.SoundType
import com.somnia.app.data.Settings

/**
 * Apprentissage par corrections : quand l'utilisateur dit « ce n'était pas X
 * mais Y », on rend X un peu moins facile à déclencher et Y un peu plus.
 * Renvoie les nouveaux réglages et une phrase qui explique le changement.
 */
object Feedback {
    fun adjust(s: Settings, wrong: SoundType, right: SoundType): Pair<Settings, String?> {
        if (wrong == right) return s to null
        val sens = s.typeSensitivity.toMutableMap()
        val notes = ArrayList<String>()
        if (wrong != SoundType.NOISE) {
            val old = sens[wrong] ?: 5
            val new = (old - 1).coerceAtLeast(1)
            if (new != old) { sens[wrong] = new; notes += "${wrong.label} $old → $new" }
        }
        if (right != SoundType.NOISE) {
            val old = sens[right] ?: 5
            val new = (old + 1).coerceAtMost(10)
            if (new != old) { sens[right] = new; notes += "${right.label} $old → $new" }
        }
        return s.copy(typeSensitivity = sens) to notes.takeIf { it.isNotEmpty() }?.joinToString(", ", prefix = "Réglage ajusté : ")
    }
}
