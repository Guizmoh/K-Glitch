package com.somnia.app.ui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/** L'icône ne change jamais pendant qu'on touche le policier, seulement en quittant l'appli. */
class AgentIconTest {
    private class Recorder : Agent.IconSwitcher {
        val calls = ArrayList<Int>()
        override fun enableOnly(index: Int) { calls += index }
    }
    private val ctx = android.app.Application()

    @Test fun iconChangesOnlyWhenLeavingTheApp() {
        Agent.applyPendingIcon(ctx, Recorder()) // état propre
        val r = Recorder()
        Agent.requestIcon(1)
        Agent.requestIcon(0)
        Agent.requestIcon(1) // plusieurs touches de suite
        assertEquals("rien ne doit changer pendant l'utilisation", emptyList<Int>(), r.calls)
        Agent.applyPendingIcon(ctx, r)
        assertEquals("une seule bascule, vers le dernier policier choisi", listOf(1), r.calls)
        Agent.applyPendingIcon(ctx, r)
        assertEquals("pas de bascule en double", listOf(1), r.calls)
        assertNull(Agent.pendingIcon)
    }
}
