package com.somnia.app.analysis

import com.somnia.app.data.SleepSession
import kotlin.math.abs
import kotlin.math.roundToInt

/** Étiquettes du journal du soir. */
object JournalTags {
    val ALL = listOf(
        "Caféine", "Alcool", "Sport", "Repas tardif", "Stress", "Écrans tard",
        "Sieste", "Médicament", "Malade", "Bruit extérieur",
    )
}

/**
 * Compare le score moyen des nuits avec / sans chaque étiquette pour
 * repérer ce qui améliore ou dégrade les nuits.
 */
object Insights {
    data class TagEffect(val tag: String, val withAvg: Int, val withoutAvg: Int, val nights: Int) {
        val delta get() = withAvg - withoutAvg
    }

    fun tagEffects(sessions: List<SleepSession>): List<TagEffect> {
        val scored = sessions.filter { it.score != null }
        return JournalTags.ALL.mapNotNull { tag ->
            val with = scored.filter { tag in it.tags.split(",") }
            val without = scored - with.toSet()
            if (with.size < 2 || without.isEmpty()) null else TagEffect(
                tag,
                with.map { it.score!! }.average().roundToInt(),
                without.map { it.score!! }.average().roundToInt(),
                with.size,
            )
        }.sortedByDescending { abs(it.delta) }
    }

    fun advice(s: SleepSession, goalMin: Int): List<String> {
        val tips = ArrayList<String>()
        val asleep = s.deepMinutes + s.lightMinutes
        if (asleep in 1 until goalMin - 60) tips += "Nuit courte : essayez de vous coucher ${(goalMin - asleep) / 60}h${"%02d".format((goalMin - asleep) % 60)} plus tôt."
        if (s.snoringMinutes > 30) tips += "Ronflements fréquents : dormir sur le côté, éviter l'alcool le soir. S'ils sont forts avec des pauses respiratoires, parlez-en à un médecin (apnée du sommeil)."
        if (s.awakenings >= 4) tips += "Sommeil fragmenté : vérifiez le bruit, la lumière et la température (idéalement 16–19 °C)."
        if ((s.avgNoiseFloorDb ?: -90f) > -45f) tips += "Chambre plutôt bruyante : des bouchons d'oreilles ou un bruit blanc peuvent aider."
        if ("Caféine" in s.tags) tips += "La caféine agit jusqu'à 6 h : évitez-la après 14 h."
        if ("Écrans tard" in s.tags) tips += "Coupez les écrans 30 à 60 min avant le coucher."
        return tips
    }
}
