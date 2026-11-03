package com.somnia.app.ui.components

import androidx.compose.animation.Crossfade
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.draw.scale
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.R
import kotlin.math.sin

private val GOLD = Color(0xFFE0B656)

/**
 * L'agent du Dodo dans son médaillon.
 * - [awake] = true : l'agent moustachu, yeux grands ouverts (avant la nuit, au réveil).
 * - [awake] = false : il dort, des « Zzz » s'envolent (pendant la nuit).
 */
@Composable
fun PoliceLogo(size: Dp = 56.dp, awake: Boolean = true, animated: Boolean = true) {
    val inf = rememberInfiniteTransition(label = "agent")
    val t by inf.animateFloat(0f, 1f, infiniteRepeatable(tween(2600, easing = LinearEasing), RepeatMode.Restart), label = "t")
    // Image de l'animation « bave » (6 images de 450 ms)
    val f by inf.animateFloat(0f, 6f, infiniteRepeatable(tween(6 * 450, easing = LinearEasing), RepeatMode.Restart), label = "frame")
    Crossfade(awake, label = "eyes") { a ->
        PoliceLogoFrame(
            size, if (a) 1f else 0f, if (!a) (if (animated) t else 0.4f) else null,
            awakeBob = if (a && animated) t else null, sleepFrame = if (animated) f.toInt().coerceIn(0, 5) else 0,
        )
    }
}

/**
 * Image fixe (pour le tuto et les captures) : [open] > 0.5 = éveillé,
 * [zzzPhase] = position des « Zzz » (0..1) ou null.
 */
@Composable
fun PoliceLogoFrame(size: Dp, open: Float, zzzPhase: Float? = null, awakeBob: Float? = null, sleepFrame: Int = 0) {
    val awake = open > 0.5f
    val agent = com.somnia.app.ui.LocalAgent.current
    val toggle = com.somnia.app.ui.LocalAgentToggle.current
    Box(Modifier.size(size).then(if (toggle != null) Modifier.clip(CircleShape).clickable(onClickLabel = "Changer de policier") { toggle() } else Modifier)) {
        Box(
            Modifier.size(size).clip(CircleShape)
                .background(Brush.radialGradient(listOf(Color(0xFF4A62C2), Color(0xFF1C2A5E))))
                .border((size.value / 60f).coerceAtLeast(1f).dp, GOLD.copy(alpha = 0.8f), CircleShape),
            contentAlignment = Alignment.Center,
        ) {
            Image(
                painterResource(
                    when {
                        agent == 1 && awake -> com.somnia.app.ui.Agent.awakeDrawable(
                            com.somnia.app.ui.LocalHour.current ?: java.util.Calendar.getInstance().get(java.util.Calendar.HOUR_OF_DAY),
                        )
                        agent == 1 -> com.somnia.app.ui.Agent.SLEEP_SEQUENCE[sleepFrame.coerceIn(0, 5)]
                        awake -> R.drawable.agent_awake
                        else -> R.drawable.agent_asleep
                    },
                ),
                if (awake) "L'agent du Dodo, éveillé" else "L'agent du Dodo endormi",
                Modifier.size(size).scale(if (agent == 1) 0.96f else 1.18f).offset(y = size * (if (agent == 1) 0.07f else 0.06f))
                    .rotate(awakeBob?.let { sin(it * 2 * Math.PI).toFloat() * 2f } ?: 0f),
                contentScale = ContentScale.Fit,
            )
        }
        zzzPhase?.let { Zzz(size, it) }
    }
}

/** Trois « Z » qui montent en grossissant puis s'effacent, au-dessus de l'épaule droite. */
@Composable
private fun Zzz(size: Dp, phase: Float) {
    for (i in 0 until 3) {
        val p = (phase + i / 3f) % 1f
        val alpha = when {
            p < 0.15f -> p / 0.15f
            p > 0.7f -> (1f - p) / 0.3f
            else -> 1f
        }
        Text(
            "Z",
            Modifier.offset(x = size * (0.72f + 0.22f * p), y = size * (0.18f - 0.32f * p)).alpha(alpha),
            color = Color.White, fontWeight = FontWeight.Black,
            fontSize = (size.value * (0.16f + 0.12f * p)).sp,
            style = androidx.compose.ui.text.TextStyle(shadow = androidx.compose.ui.graphics.Shadow(Color(0xFF0C1135), Offset(2f, 2f), 4f)),
        )
    }
}

private val NIGHT_FRAMES = arrayOf(
    intArrayOf(
        R.drawable.night_agent1_f0, R.drawable.night_agent1_f1, R.drawable.night_agent1_f2, R.drawable.night_agent1_f3,
        R.drawable.night_agent1_f4, R.drawable.night_agent1_f5, R.drawable.night_agent1_f6,
    ),
    intArrayOf(
        R.drawable.night_agent2_f0, R.drawable.night_agent2_f1, R.drawable.night_agent2_f2, R.drawable.night_agent2_f3,
        R.drawable.night_agent2_f4, R.drawable.night_agent2_f5, R.drawable.night_agent2_f6,
    ),
)

/**
 * Animation de nuit (7 images de 420 ms en boucle, comme le GIF d'origine) :
 * l'agent choisi dort dans son cadre, avec ses « Zzz » et sa bave.
 * [frame] force une image (captures d'écran).
 */
@Composable
fun NightAgentAnimation(width: Dp, frame: Int? = null) {
    val agent = com.somnia.app.ui.LocalAgent.current.coerceIn(0, 1)
    val inf = rememberInfiniteTransition(label = "night")
    val f by inf.animateFloat(0f, 7f, infiniteRepeatable(tween(7 * 420, easing = LinearEasing), RepeatMode.Restart), label = "f")
    val i = (frame ?: f.toInt()).coerceIn(0, 6)
    val toggle = com.somnia.app.ui.LocalAgentToggle.current
    Image(
        painterResource(NIGHT_FRAMES[agent][i]), "L'agent du Dodo dort",
        Modifier.size(width = width, height = width * (361f / 430f))
            .then(if (toggle != null) Modifier.clickable(onClickLabel = "Changer de policier") { toggle() } else Modifier),
        contentScale = ContentScale.Fit,
    )
}
