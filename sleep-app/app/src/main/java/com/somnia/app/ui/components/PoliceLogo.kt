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
    Crossfade(awake, label = "eyes") { a ->
        PoliceLogoFrame(size, if (a) 1f else 0f, if (!a) (if (animated) t else 0.4f) else null, awakeBob = if (a && animated) t else null)
    }
}

/**
 * Image fixe (pour le tuto et les captures) : [open] > 0.5 = éveillé,
 * [zzzPhase] = position des « Zzz » (0..1) ou null.
 */
@Composable
fun PoliceLogoFrame(size: Dp, open: Float, zzzPhase: Float? = null, awakeBob: Float? = null) {
    val awake = open > 0.5f
    Box(Modifier.size(size)) {
        Box(
            Modifier.size(size).clip(CircleShape)
                .background(Brush.radialGradient(listOf(Color(0xFF3A4DB0), Color(0xFF0C1135))))
                .border((size.value / 60f).coerceAtLeast(1f).dp, GOLD.copy(alpha = 0.8f), CircleShape),
            contentAlignment = Alignment.Center,
        ) {
            Image(
                painterResource(if (awake) R.drawable.agent_awake else R.drawable.agent_asleep),
                if (awake) "L'agent du Dodo, éveillé" else "L'agent du Dodo endormi",
                Modifier.size(size).scale(1.18f).offset(y = size * 0.06f)
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
