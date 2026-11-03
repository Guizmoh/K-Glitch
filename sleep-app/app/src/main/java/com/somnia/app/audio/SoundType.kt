package com.somnia.app.audio

/** Catégories de sons nocturnes que l'application sait reconnaître. */
enum class SoundType(val label: String, val emoji: String) {
    SPEECH("Paroles", "🗣️"),
    NIGHT_TERROR("Cris / terreur nocturne", "😱"),
    SNORING("Ronflement", "😴"),
    LAUGHTER("Rire", "😂"),
    CRYING("Pleurs", "😢"),
    COUGH("Toux / éternuement", "🤧"),
    BREATHING("Respiration bruyante", "💨"),
    MEDIA("Télé / musique", "📺"),
    NOISE("Autre bruit", "🔊");

    companion object {
        fun fromName(name: String): SoundType = entries.firstOrNull { it.name == name } ?: NOISE
    }
}

/** Score de chaque catégorie (0..1) pour une fenêtre audio. */
typealias SoundScores = Map<SoundType, Float>
