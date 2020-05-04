// QFontDialogTester Demo by Codebind.com
// copied from http://www.codebind.com/cpp-tutorial/qt-tutorial/qt-tutorials-for-beginners-qfontdialog-example/
// license is not known
#include <QApplication>
#include <QDebug>
#include <QFontDialog>

class QFontDialogTester : public QWidget
{
public:
  void onFont()
  {
    bool ok;
    QFont font = QFontDialog::getFont(
                   &ok,
                   QFont( "Arial", 18 ),
                   this,
                   tr("Pick a font") );
    if( ok )
      {
        qDebug() << "font           : " << font;
        qDebug() << "font weight    : " << font.weight();
        qDebug() << "font family    : " << font.family();
        qDebug() << "font style     : " << font.style();
        //  StyleNormal = 0, StyleItalic = 1, StyleOblique = 2
        qDebug() << "font pointSize : " << font.pointSize();
        qDebug() << "font string : " << font.toString();
      }
  }
};

int main( int argc, char **argv )
{
  QApplication app( argc, argv );
  QFontDialogTester font_test;
  font_test.onFont();
  return 0;
}

///

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "g++ -o qfontdialog-example -Wall -fPIC -O $(pkg-config --cflags Qt5Widgets) -g qfontdialog-example.cc $(pkg-config --libs Qt5Widgets)" ;;
 ** End: ;;
 **
 ** indentable using: style -v -s2 --style=gnu qfontdialog-example.cc
 ****************/
