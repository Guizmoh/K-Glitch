package com.somnia.app.ui.components

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.clipPath
import androidx.compose.ui.graphics.drawscope.translate
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.graphics.vector.PathParser
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlin.random.Random

private class ParsedLayer(val path: Path, val layer: LogoArt.Layer)

private val parsedLayers by lazy {
    LogoArt.layers.map { ParsedLayer(PathParser().parsePathString(it.path).toPath(), it) }
}
private val zzzPaths by lazy { LogoArt.zzz.map { PathParser().parsePathString(it).toPath() } }

private val GOLD_RING = Color(0xBFE0B656)
private val BG_CENTER = Color(0xFF3A4DB0)
private val BG_EDGE = Color(0xFF0C1135)

/**
 * Logo animé du policier du sommeil.
 *
 * - [awake] = true : yeux ouverts, il cligne de temps en temps
 *   (avant de lancer la nuit, au réveil).
 * - [awake] = false : il dort, des « Zzz » s'envolent (pendant la nuit).
 * Le passage d'un état à l'autre ouvre ou ferme doucement les paupières.
 */
@Composable
fun PoliceLogo(size: Dp = 56.dp, awake: Boolean = true, animated: Boolean = true) {
    val openness = remember { Animatable(if (awake) 1f else 0f) }
    LaunchedEffect(awake, animated) {
        if (!animated) { openness.snapTo(if (awake) 1f else 0f); return@LaunchedEffect }
        if (awake) {
            openness.animateTo(1f, tween(700, easing = FastOutSlowInEasing))
            while (true) {
                delay(Random.nextLong(2500, 5500))
                openness.animateTo(0f, tween(80))
                openness.animateTo(1f, tween(120))
                if (Random.nextInt(4) == 0) { // double clignement de temps en temps
                    delay(150)
                    openness.animateTo(0f, tween(80))
                    openness.animateTo(1f, tween(120))
                }
            }
        } else {
            openness.animateTo(0f, tween(900, easing = FastOutSlowInEasing))
        }
    }
    val zzz = rememberInfiniteTransition(label = "zzz")
    val drift by zzz.animateFloat(0f, 1f, infiniteRepeatable(tween(2600, easing = LinearEasing), RepeatMode.Restart), label = "drift")

    PoliceLogoFrame(size, openness.value, if (!awake && openness.value < 0.3f) (if (animated) drift else 0.5f) else null, awake)
}

/**
 * Une image fixe du logo : [open] = ouverture des yeux (0..1),
 * [zzzPhase] = position des « Zzz » (0..1) ou null pour ne pas les dessiner.
 */
@Composable
fun PoliceLogoFrame(size: Dp, open: Float, zzzPhase: Float? = null, awake: Boolean = open > 0.5f) {
    Canvas(Modifier.size(size).semantics { contentDescription = if (awake) "Policier du Dodo éveillé" else "Policier du Dodo endormi" }) {
        val s = this.size.minDimension / 72f // le médaillon montre le cercle de rayon 36 du repère 108
        withTransform({
            scale(s, s, Offset.Zero)
            translate(-18f, -18f)
        }) {
            drawMedallion(open)
            zzzPhase?.let { drawZzz(it) }
        }
    }
}

private fun DrawScope.drawMedallion(open: Float) {
    val center = Offset(54f, 54f)
    val circle = Path().apply { addOval(Rect(center, 36f)) }
    clipPath(circle) {
        drawCircle(Brush.radialGradient(listOf(BG_CENTER, BG_EDGE), Offset(54f, 43f), 52f), 36f, center)
        drawCircle(GOLD_RING, 34.5f, center, style = Stroke(1.1f))
        for (p in parsedLayers) {
            val l = p.layer
            translate(0f, l.dy) {
                l.grad?.let { g ->
                    drawPath(p.path, Brush.linearGradient(listOf(g.from, g.to), Offset(g.x1, g.y1), Offset(g.x2, g.y2)))
                }
                l.fill?.let { drawPath(p.path, it) }
                l.stroke?.let { drawPath(p.path, it, style = Stroke(l.width, cap = StrokeCap.Round, join = StrokeJoin.Round)) }
            }
        }
        drawEyes(open)
    }
}

private fun DrawScope.drawEyes(open: Float) {
    val rx = LogoArt.EYE_RX
    val ry = LogoArt.EYE_RY
    for ((cx, cy) in LogoArt.eyeCenters) {
        if (open < 0.12f) {
            // Paupières fermées : petit arc souriant
            val p = Path().apply { moveTo(cx - rx, cy); quadraticTo(cx, cy + 2.4f, cx + rx, cy) }
            drawPath(p, LogoArt.lid, style = Stroke(1.5f, cap = StrokeCap.Round))
            continue
        }
        val h = ry * open
        val eye = Path().apply { addOval(Rect(Offset(cx - rx, cy - h), Size(2 * rx, 2 * h))) }
        drawPath(eye, LogoArt.eyeWhite)
        clipPath(eye) {
            drawCircle(LogoArt.iris, LogoArt.IRIS_R, Offset(cx, cy + 0.2f))
            drawCircle(LogoArt.eyeWhite, 0.5f, Offset(cx + 0.55f, cy - 0.45f))
        }
        // Paupière supérieure qui suit l'ouverture
        val lid = Path().apply {
            moveTo(cx - rx - 0.2f, cy - 0.3f * open)
            quadraticTo(cx, cy - h - 1.3f * open, cx + rx + 0.2f, cy - 0.3f * open)
        }
        drawPath(lid, LogoArt.lid, style = Stroke(1.1f, cap = StrokeCap.Round))
    }
}

/** Les « Zzz » montent et s'effacent, en débordant du médaillon. */
private fun DrawScope.drawZzz(t: Float) {
    zzzPaths.forEachIndexed { i, p ->
        val phase = (t + i * 0.5f) % 1f
        val alpha = when {
            phase < 0.2f -> phase / 0.2f
            phase > 0.75f -> (1f - phase) / 0.25f
            else -> 1f
        }
        translate(6f * phase, -6f * phase) {
            drawPath(p, Color.White.copy(alpha = 0.9f * alpha), style = Stroke(1.6f, cap = StrokeCap.Round, join = StrokeJoin.Round))
        }
    }
}
