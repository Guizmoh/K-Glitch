package com.somnia.app.audio

import android.content.Context
import com.google.mediapipe.tasks.audio.audioclassifier.AudioClassifier
import com.google.mediapipe.tasks.audio.core.RunningMode
import com.google.mediapipe.tasks.components.containers.AudioData
import com.google.mediapipe.tasks.core.BaseOptions

/**
 * Classifieur basé sur YAMNet (réseau de neurones entraîné sur AudioSet,
 * 521 classes). Tout tourne sur le téléphone, rien n'est envoyé en ligne.
 */
class YamnetClassifier private constructor(private val classifier: AudioClassifier) : SoundClassifier {
    override val name = "YAMNet (IA embarquée)"

    override fun classify(window: FloatArray): SoundScores {
        val format = AudioData.AudioDataFormat.builder()
            .setNumOfChannels(1)
            .setSampleRate(AudioMath.SAMPLE_RATE.toFloat())
            .build()
        val data = AudioData.create(format, window.size)
        data.load(window)
        val result = classifier.classify(data)
        val scores = HashMap<SoundType, Float>()
        for (cr in result.classificationResults()) {
            for (c in cr.classifications()) {
                for (cat in c.categories()) {
                    val type = MAPPING[cat.categoryName()] ?: continue
                    scores[type] = maxOf(scores[type] ?: 0f, cat.score())
                }
            }
        }
        return scores
    }

    override fun close() = classifier.close()

    companion object {
        private const val MODEL = "yamnet.tflite"

        /** Correspondance entre les classes AudioSet et nos catégories. */
        val MAPPING: Map<String, SoundType> = buildMap {
            listOf(
                "Speech", "Child speech, kid speaking", "Conversation", "Narration, monologue",
                "Babbling", "Whispering", "Male speech, man speaking", "Female speech, woman speaking",
            ).forEach { put(it, SoundType.SPEECH) }
            listOf(
                "Shout", "Bellow", "Yell", "Screaming", "Children shouting", "Whoop", "Battle cry",
            ).forEach { put(it, SoundType.NIGHT_TERROR) }
            listOf(
                "Laughter", "Baby laughter", "Giggle", "Snicker", "Belly laugh", "Chuckle, chortle",
            ).forEach { put(it, SoundType.LAUGHTER) }
            listOf(
                "Crying, sobbing", "Baby cry, infant cry", "Whimper", "Wail, moan",
            ).forEach { put(it, SoundType.CRYING) }
            listOf("Snoring", "Snort").forEach { put(it, SoundType.SNORING) }
            listOf("Cough", "Sneeze", "Throat clearing").forEach { put(it, SoundType.COUGH) }
            listOf("Breathing", "Wheeze", "Gasp", "Pant", "Sigh", "Groan", "Grunt")
                .forEach { put(it, SoundType.BREATHING) }
        }

        fun create(context: Context): YamnetClassifier? = runCatching {
            context.assets.open(MODEL).close()
            val options = AudioClassifier.AudioClassifierOptions.builder()
                .setBaseOptions(BaseOptions.builder().setModelAssetPath(MODEL).build())
                .setRunningMode(RunningMode.AUDIO_CLIPS)
                .setMaxResults(25)
                .build()
            YamnetClassifier(AudioClassifier.createFromOptions(context, options))
        }.getOrNull()
    }
}
