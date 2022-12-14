// file misc-basile/winpersist.c
// SPDX-License-Identifier: GPL-3.0-or-later
// From https://www.manpagez.com/html/gtk3/gtk3-3.14.4/gtk3-General.php#gtk-parse-args
#include <stdlib.h>
#include <gtk/gtk.h>

int
main (int argc, char **argv)
{
  GtkWidget *win = NULL, *but = NULL;
  const char *text = "Close yourself. I mean it!";

  gtk_init (&argc, &argv);

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (win, "delete-event", G_CALLBACK (gtk_true), NULL);
  g_signal_connect (win, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  but = gtk_button_new_with_label (text);
  g_signal_connect_swapped (but, "clicked",
			    G_CALLBACK (gtk_widget_destroy), win);
  gtk_container_add (GTK_CONTAINER (win), but);

  gtk_widget_show_all (win);

  gtk_main ();

  return 0;
}


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -Wall -Og -g3 $(pkg-config --cflags gtk+-3.0) winpersist.c $(pkg-config --libs gtk+-3.0) -o winpersist" ;;
 ** End: ;;
 ****************/
