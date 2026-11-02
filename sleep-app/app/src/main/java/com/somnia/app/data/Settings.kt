package com.somnia.app.data

import android.content.Context
import com.somnia.app.audio.EventClassifier
import com.somnia.app.audio.SoundType
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

data class Settings(
    /** Micro utilisé (voir MicChoice) : auto, bottom, back, all, dev:ID. */
    val micSource: String = "auto",
    /** Personnage : 0 = l'agent moustachu, 1 = le gros agent au nez rouge. */
    val agent: Int = 0,
    /** Sensibilité globale du micro 1..10 (écart nécessaire avec le bruit de fond). */
    val micSensitivity: Int = 6,
    /** Intensité de captation par catégorie 1..10 (10 = capte les moindres signes). */
    val typeSensitivity: Map<SoundType, Int> = defaultTypeSensitivity(),
    /** Catégories dont on conserve l'enregistrement audio. */
    val recordTypes: Set<SoundType> = SoundType.entries.toSet() - SoundType.NOISE,
    /** Créer automatiquement une version « voix isolée » des paroles, cris, rires, pleurs. */
    val enhanceVoice: Boolean = true,
    /** Ne commencer la détection qu'après N minutes (temps d'endormissement). */
    val startDelayMin: Int = 0,
    val trackMovement: Boolean = true,
    /** Suppression automatique des enregistrements (hors favoris) après N jours, 0 = jamais. */
    val retentionDays: Int = 30,
    val sleepGoalMin: Int = 8 * 60,
    // Réveil intelligent
    val smartAlarmEnabled: Boolean = false,
    val alarmHour: Int = 7,
    val alarmMinute: Int = 0,
    val alarmWindowMin: Int = 30,
    // Rappel du coucher
    val bedtimeReminderEnabled: Boolean = false,
    val bedtimeHour: Int = 22,
    val bedtimeMinute: Int = 30,
) {
    fun thresholds(): Map<SoundType, Float> {
        val base = EventClassifier.defaultThresholds().toMutableMap()
        for ((t, lvl) in typeSensitivity) base[t] = EventClassifier.thresholdFromLevel(lvl)
        return base
    }

    companion object {
        fun defaultTypeSensitivity(): Map<SoundType, Int> = mapOf(
            SoundType.SPEECH to 7,
            SoundType.NIGHT_TERROR to 7,
            SoundType.SNORING to 7,
            SoundType.LAUGHTER to 7,
            SoundType.CRYING to 7,
            SoundType.COUGH to 5,
            SoundType.BREATHING to 5,
        )
    }
}

/** Modes de captation prêts à l'emploi (réglage simple). */
enum class CapturePreset(val label: String, val description: String, val mic: Int, val types: Int) {
    DISCREET("Discret", "Seulement les sons forts et nets : cris, paroles distinctes, gros ronflements.", 4, 4),
    NORMAL("Normal", "Le bon équilibre pour la plupart des chambres.", 6, 7),
    ALL("Tout capter", "Chuchotements, petits rires, légers sanglots… plus de fausses alertes.", 9, 9);

    fun apply(s: Settings) = s.copy(
        micSensitivity = mic,
        typeSensitivity = s.typeSensitivity.mapValues { (t, _) ->
            if (t == SoundType.BREATHING || t == SoundType.COUGH) (types - 2).coerceAtLeast(1) else types
        },
    )

    companion object {
        fun of(s: Settings): CapturePreset? = entries.firstOrNull { it.apply(s) == s }
    }
}

/** Stockage simple des réglages dans les SharedPreferences, exposés en StateFlow. */
class SettingsRepository private constructor(context: Context) {
    private val prefs = context.getSharedPreferences("settings", Context.MODE_PRIVATE)
    private val _settings = MutableStateFlow(load())
    val settings: StateFlow<Settings> = _settings

    fun update(transform: (Settings) -> Settings) {
        val s = transform(_settings.value)
        save(s)
        _settings.value = s
    }

    private fun load(): Settings {
        val d = Settings()
        return Settings(
            micSensitivity = prefs.getInt("mic", d.micSensitivity),
            micSource = prefs.getString("mic_source", d.micSource) ?: "auto",
            agent = prefs.getInt("agent", d.agent),
            typeSensitivity = SoundType.entries.filter { it != SoundType.NOISE }.associateWith {
                prefs.getInt("sens_${it.name}", d.typeSensitivity[it] ?: 5)
            },
            recordTypes = prefs.getStringSet("record", null)?.map { SoundType.fromName(it) }?.toSet() ?: d.recordTypes,
            enhanceVoice = prefs.getBoolean("enhance", d.enhanceVoice),
            startDelayMin = prefs.getInt("delay", d.startDelayMin),
            trackMovement = prefs.getBoolean("movement", d.trackMovement),
            retentionDays = prefs.getInt("retention", d.retentionDays),
            sleepGoalMin = prefs.getInt("goal", d.sleepGoalMin),
            smartAlarmEnabled = prefs.getBoolean("alarm", d.smartAlarmEnabled),
            alarmHour = prefs.getInt("alarm_h", d.alarmHour),
            alarmMinute = prefs.getInt("alarm_m", d.alarmMinute),
            alarmWindowMin = prefs.getInt("alarm_w", d.alarmWindowMin),
            bedtimeReminderEnabled = prefs.getBoolean("bed", d.bedtimeReminderEnabled),
            bedtimeHour = prefs.getInt("bed_h", d.bedtimeHour),
            bedtimeMinute = prefs.getInt("bed_m", d.bedtimeMinute),
        )
    }

    private fun save(s: Settings) {
        prefs.edit().apply {
            putInt("mic", s.micSensitivity)
            putString("mic_source", s.micSource)
            putInt("agent", s.agent)
            s.typeSensitivity.forEach { (t, v) -> putInt("sens_${t.name}", v) }
            putStringSet("record", s.recordTypes.map { it.name }.toSet())
            putBoolean("enhance", s.enhanceVoice)
            putInt("delay", s.startDelayMin)
            putBoolean("movement", s.trackMovement)
            putInt("retention", s.retentionDays)
            putInt("goal", s.sleepGoalMin)
            putBoolean("alarm", s.smartAlarmEnabled)
            putInt("alarm_h", s.alarmHour)
            putInt("alarm_m", s.alarmMinute)
            putInt("alarm_w", s.alarmWindowMin)
            putBoolean("bed", s.bedtimeReminderEnabled)
            putInt("bed_h", s.bedtimeHour)
            putInt("bed_m", s.bedtimeMinute)
        }.apply()
    }

    companion object {
        @Volatile private var instance: SettingsRepository? = null
        fun get(context: Context) = instance ?: synchronized(this) {
            instance ?: SettingsRepository(context.applicationContext).also { instance = it }
        }
    }
}
