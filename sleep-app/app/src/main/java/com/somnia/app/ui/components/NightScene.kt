package com.somnia.app.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import kotlin.math.cos
import kotlin.math.sin
import kotlin.random.Random

// Palette inspirée de l'affiche (nuit bleue, lumières chaudes, feuillage éclairé)
private val SKY_TOP = Color(0xFF233773)
private val SKY_LOW = Color(0xFF4262AE)
private val CLOUD_DARK = Color(0xFF5068AA)
private val CLOUD_LIGHT = Color(0xFFB4C3EA)
private val CITY_FAR = Color(0xFF3A5096)
private val CITY_NEAR = Color(0xFF26397A)
private val FILL_BOTTOM = Color(0xFF213268)
private val WINDOW = Color(0xFFF2C46B)
private val LEAF_DARK = listOf(Color(0xFF1C3A2E), Color(0xFF244634), Color(0xFF2D533A))
private val LEAF_LIT = listOf(Color(0xFF3F6431), Color(0xFF5E8236), Color(0xFF8DA640), Color(0xFFB8C25A))

/**
 * Décor « Ghibli » de la fenêtre du dodo : nuages en haut, silhouette de la
 * ville dessous (son aplat accueille les premiers textes), feuillages en bas
 * et sur les côtés jusqu'à l'horloge.
 *
 * [roofLine] = hauteur où commence l'aplat des bâtiments,
 * [foliageTop] = hauteur jusqu'où montent les feuillages sur les côtés.
 */
@Composable
fun NightScene(roofLine: Dp = 150.dp, foliageTop: Dp = 300.dp, modifier: Modifier = Modifier.fillMaxSize()) {
    Canvas(modifier) {
        val rnd = Random(7)
        val roof = roofLine.toPx()
        // Ciel
        drawRect(Brush.verticalGradient(listOf(SKY_TOP, SKY_LOW), 0f, roof))
        drawStars(rnd, roof * 0.75f)
        drawMoon(Offset(size.width * 0.80f, 46.dp.toPx()), 18.dp.toPx())
        drawClouds(roof)
        // Ville lointaine puis proche
        drawCity(Random(3), roof - 4.dp.toPx(), 52.dp.toPx(), 92.dp.toPx(), CITY_FAR, windows = 0.10f)
        drawCity(Random(11), roof, 22.dp.toPx(), 62.dp.toPx(), CITY_NEAR, windows = 0.25f)
        // Aplat de la silhouette (fond des textes)
        drawRect(Brush.verticalGradient(listOf(CITY_NEAR, FILL_BOTTOM), roof, size.height), Offset(0f, roof), Size(size.width, size.height - roof))
        drawLamp(Offset(14.dp.toPx(), roof - 44.dp.toPx()))
        drawFoliage(foliageTop.toPx())
    }
}

private fun DrawScope.drawStars(rnd: Random, maxY: Float) {
    repeat(28) {
        val p = Offset(rnd.nextFloat() * size.width, rnd.nextFloat() * maxY)
        val r = (0.6f + rnd.nextFloat() * 1.2f) * density
        drawCircle(Color(0xFFFFF4D6).copy(alpha = 0.35f + rnd.nextFloat() * 0.5f), r, p)
    }
}

private fun DrawScope.drawMoon(c: Offset, r: Float) {
    drawCircle(Brush.radialGradient(listOf(Color(0x55F6E7B0), Color.Transparent), c, r * 4), r * 4, c)
    val full = Path().apply { addOval(Rect(c, r)) }
    val bite = Path().apply { addOval(Rect(Offset(c.x - r * 0.45f, c.y - r * 0.2f), r * 0.86f)) }
    drawPath(Path.combine(androidx.compose.ui.graphics.PathOperation.Difference, full, bite), Color(0xFFF6E7B0))
}

/** Nuages en boules, bord supérieur éclairé par la lune. */
private fun DrawScope.drawClouds(roof: Float) {
    val clouds = listOf(
        Triple(0.10f, 0.10f, 0.9f), Triple(0.46f, 0.07f, 0.7f), Triple(0.95f, 0.14f, 0.8f),
        Triple(0.62f, 0.24f, 0.55f), Triple(0.30f, 0.26f, 0.5f),
    )
    for ((fx, fy, sc) in clouds) {
        val base = Offset(size.width * fx, roof * fy)
        val u = 16.dp.toPx() * sc
        val blobs = listOf(-2.2f to 0.5f, -1.2f to -0.2f, 0f to -0.6f, 1.3f to -0.1f, 2.4f to 0.5f, 0.6f to 0.6f, -0.8f to 0.7f)
        for ((bx, by) in blobs) drawCircle(Color(0x33000820), u, Offset(base.x + bx * u + u * 0.1f, base.y + by * u + u * 0.3f))
        for ((bx, by) in blobs) drawCircle(CLOUD_LIGHT, u * 1.04f, Offset(base.x + bx * u - u * 0.12f, base.y + by * u - u * 0.22f))
        for ((bx, by) in blobs) drawCircle(CLOUD_DARK, u * 0.97f, Offset(base.x + bx * u + u * 0.05f, base.y + by * u + u * 0.04f))
        drawRect(CLOUD_DARK, Offset(base.x - 2.3f * u, base.y + 0.45f * u), Size(4.6f * u, 0.55f * u))
    }
}

/** Silhouette de toits : maisons à pignon, tours à toit conique, clocher. */
private fun DrawScope.drawCity(rnd: Random, base: Float, minH: Float, maxH: Float, color: Color, windows: Float) {
    val path = Path().apply { moveTo(0f, base) }
    val lit = ArrayList<Rect>()
    var x = -10f
    while (x < size.width + 10f) {
        val w = (26 + rnd.nextInt(34)).dp.toPx()
        val h = minH + rnd.nextFloat() * (maxH - minH)
        val top = base - h
        path.lineTo(x, top)
        when (rnd.nextInt(5)) {
            0, 1 -> path.lineTo(x + w / 2, top - w * 0.45f) // pignon
            2 -> { // tour à toit conique (comme sur l'affiche)
                path.lineTo(x + w * 0.1f, top); path.lineTo(x + w / 2, top - w * 1.1f); path.lineTo(x + w * 0.9f, top)
            }
            3 -> { // clocher pointu
                path.lineTo(x + w * 0.3f, top); path.lineTo(x + w * 0.3f, top - w * 0.6f)
                path.lineTo(x + w / 2, top - w * 1.9f); path.lineTo(x + w * 0.7f, top - w * 0.6f); path.lineTo(x + w * 0.7f, top)
            }
            else -> Unit // toit plat
        }
        path.lineTo(x + w, top)
        // Fenêtres allumées
        val ww = 4.dp.toPx(); val wh = 6.dp.toPx()
        var wy = top + 10.dp.toPx()
        while (wy < base - wh - 10.dp.toPx()) {
            var wx = x + 7.dp.toPx()
            while (wx < x + w - ww - 6.dp.toPx()) {
                if (rnd.nextFloat() < windows) lit += Rect(Offset(wx, wy), Size(ww, wh))
                wx += 10.dp.toPx()
            }
            wy += 14.dp.toPx()
        }
        x += w
    }
    path.lineTo(size.width + 10f, base); path.lineTo(size.width + 10f, base + 2f); path.lineTo(0f, base + 2f); path.close()
    drawPath(path, color)
    for (r in lit) {
        drawCircle(WINDOW.copy(alpha = 0.18f), r.width * 1.6f, r.center)
        drawRect(WINDOW.copy(alpha = 0.9f), r.topLeft, r.size)
    }
}

/** Réverbère avec halo chaud. */
private fun DrawScope.drawLamp(top: Offset) {
    val c = Offset(top.x + 10.dp.toPx(), top.y)
    drawCircle(Brush.radialGradient(listOf(Color(0x66F2C46B), Color.Transparent), c, 46.dp.toPx()), 46.dp.toPx(), c)
    drawRect(CITY_NEAR, Offset(top.x + 8.5f.dp.toPx(), top.y), Size(3.dp.toPx(), 44.dp.toPx()))
    drawRect(Color(0xFF2A2418), Offset(c.x - 6.dp.toPx(), c.y - 8.dp.toPx()), Size(12.dp.toPx(), 14.dp.toPx()))
    drawCircle(Color(0xFFFFE3A0), 4.5f.dp.toPx(), c)
}

/** Feuillage : touffes en bas, qui remontent sur les bords jusqu'à [topY]. */
private fun DrawScope.drawFoliage(topY: Float) {
    val rnd = Random(21)
    val h = size.height
    val clusters = ArrayList<Pair<Offset, Float>>()
    // Bords gauche et droit, de topY jusqu'en bas
    var y = topY
    while (y < h) {
        clusters += Offset(-6.dp.toPx() + rnd.nextFloat() * 18.dp.toPx(), y) to (34 + rnd.nextInt(18)).dp.toPx()
        clusters += Offset(size.width + 6.dp.toPx() - rnd.nextFloat() * 18.dp.toPx(), y + 20.dp.toPx()) to (34 + rnd.nextInt(18)).dp.toPx()
        y += 46.dp.toPx()
    }
    // Rangée de buissons en bas
    var x = 0f
    while (x < size.width + 40f) {
        clusters += Offset(x, h - (30 + rnd.nextInt(40)).dp.toPx()) to (44 + rnd.nextInt(24)).dp.toPx()
        x += 52.dp.toPx()
    }
    // Masse sombre puis feuilles éclairées par-dessus
    for ((c, r) in clusters) drawCircle(LEAF_DARK[0], r, c)
    for ((c, r) in clusters) {
        repeat((r / density * 1.3f).toInt()) {
            val a = rnd.nextFloat() * 6.283f
            val d = r * kotlin.math.sqrt(rnd.nextFloat())
            val p = Offset(c.x + cos(a) * d, c.y + sin(a) * d)
            // la lumière vient d'en haut : feuilles du haut plus claires
            val litness = ((c.y - p.y) / r + 0.3f + rnd.nextFloat() * 0.4f)
            val col = if (litness > 0.62f) LEAF_LIT[rnd.nextInt(LEAF_LIT.size)] else LEAF_DARK[rnd.nextInt(LEAF_DARK.size)]
            drawLeaf(p, (7 + rnd.nextInt(7)).dp.toPx(), rnd.nextFloat() * 360f, col)
        }
    }
    // Lucioles
    repeat(10) {
        val p = Offset(rnd.nextFloat() * size.width, h - rnd.nextFloat() * (h - topY))
        drawCircle(Color(0x33F2E27A), 6.dp.toPx(), p)
        drawCircle(Color(0xCCF8EC9A), 1.6f.dp.toPx(), p)
    }
}

private fun DrawScope.drawLeaf(c: Offset, len: Float, angle: Float, color: Color) {
    rotate(angle, c) {
        val p = Path().apply {
            moveTo(c.x - len / 2, c.y)
            quadraticTo(c.x, c.y - len * 0.42f, c.x + len / 2, c.y)
            quadraticTo(c.x, c.y + len * 0.42f, c.x - len / 2, c.y)
            close()
        }
        drawPath(p, color)
    }
}
