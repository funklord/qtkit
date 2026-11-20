// The tests for qtkit, run offscreen by `make test`. Plain checks rather than
// QtTest, so no crash handler can attach a debugger to a failing run.
//
// Every case builds the desktop it asks about -- HOME, the session's name and
// the files it writes -- in a directory of its own, so the machine's real
// desktop never answers.

#include "qtkit/flow_layout.h"
#include "qtkit/theme.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QStyle>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QWidget>

#include <cstdio>

namespace {

int failures = 0;
int checks = 0;

void check(bool ok, const char *what)
{
	++checks;
	if (!ok) {
		++failures;
		std::fprintf(stderr, "FAIL: %s\n", what);
	}
}

void write(const QString &path, const char *text)
{
	QDir().mkpath(path.left(path.lastIndexOf(QLatin1Char('/'))));
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		std::fprintf(stderr, "cannot write %s\n", qPrintable(path));
	file.write(text);
}

const char *dark_kdeglobals =
    "[General]\nwindowBackground=0,42,78\nwindowForeground=255,255,255\n";
const char *light_kdeglobals =
    "[General]\nwindowBackground=239,239,239\nwindowForeground=0,0,0\n";
const char *dark_lxqt =
    "[Palette]\nwindow_color=#232323\nwindow_text_color=#ffffff\n";
const char *light_lxqt =
    "[Palette]\nwindow_color=#efefef\nwindow_text_color=#101010\n";

// A desktop of our own: nothing of the machine's is read, and no bus is
// named, so the portal abstains without autolaunching one.
struct desktop {
	QTemporaryDir root;
	QString trinity() const { return root.path() + "/trinity/share/config/kdeglobals"; }
	QString lxqt() const { return root.path() + "/config/lxqt/lxqt.conf"; }

	explicit desktop(const char *session)
	{
		qputenv("HOME", root.path().toLocal8Bit());
		qputenv("TDEHOME", (root.path() + "/trinity").toLocal8Bit());
		qputenv("KDEHOME", (root.path() + "/kde").toLocal8Bit());
		qputenv("XDG_CONFIG_HOME", (root.path() + "/config").toLocal8Bit());
		qputenv("XDG_CURRENT_DESKTOP", session);
		qputenv("DBUS_SESSION_BUS_ADDRESS", "");
		qunsetenv("QTKIT_TEST_THEME");
	}
};

void test_scheme_of()
{
	check(qtkit_scheme_of(Qt::black, Qt::white) == qtkit_scheme::dark, "black on white is dark");
	check(qtkit_scheme_of(Qt::white, Qt::black) == qtkit_scheme::light, "white under black is light");
	check(qtkit_scheme_of(Qt::gray, Qt::gray) == qtkit_scheme::unknown, "identical says nothing");
	check(qtkit_scheme_of(QColor(), Qt::white) == qtkit_scheme::unknown, "invalid says nothing");
}

void test_files()
{
	QTemporaryDir dir;
	const QString a = dir.path() + "/a", b = dir.path() + "/b";
	write(a, dark_kdeglobals);
	check(qtkit_scheme_from_files({a}) == qtkit_scheme::dark, "kdeglobals triples");
	write(a, dark_lxqt);
	check(qtkit_scheme_from_files({a}) == qtkit_scheme::dark, "lxqt hex");
	write(a, "[General]\nwindowBackground=#000000\nwindowForeground=#ffffff\n");
	check(qtkit_scheme_from_files({a}) == qtkit_scheme::dark, "hex read in either dialect");
	write(a, "[Other]\nwindowBackground=0,0,0\nwindowForeground=255,255,255\n"
	         "[General]\nwindowBackground=255,255,255\nwindowForeground=0,0,0\n");
	check(qtkit_scheme_from_files({a}) == qtkit_scheme::light, "another section is not read");
	write(a, "[General]\nwindowBackground=0,0,300\nwindowForeground=255,255,255\n");
	check(qtkit_scheme_from_files({a}) == qtkit_scheme::unknown, "a channel past 255 says nothing");
	write(b, light_lxqt);
	check(qtkit_scheme_from_files({dir.path() + "/none", a, b}) == qtkit_scheme::light,
	      "the first file that answers");
}

void test_portal_gate()
{
	qputenv("DBUS_SESSION_BUS_ADDRESS", "");
	bool asked = true;
	check(qtkit_portal_scheme(&asked) == -1, "no bus, no answer");
	check(!asked, "no bus named: the bus is not touched");
}

void test_order()
{
	desktop tde("TDE");
	write(tde.trinity(), dark_kdeglobals);
	write(tde.lxqt(), light_lxqt);
	check(qtkit_desktop_scheme() == qtkit_scheme::dark, "the running session's file first (TDE)");
	qputenv("XDG_CURRENT_DESKTOP", "LXQt");
	check(qtkit_desktop_scheme() == qtkit_scheme::light, "the running session's file first (LXQt)");
	qputenv("XDG_CURRENT_DESKTOP", "");
	check(qtkit_desktop_scheme() == qtkit_scheme::dark, "no session named: any file before the palette");
	QFile::remove(tde.trinity());
	QFile::remove(tde.lxqt());
	check(qtkit_desktop_scheme() == qtkit_scheme::light, "no file: the palette Qt gave, light here");
	qputenv("QTKIT_TEST_THEME", "dark");
	check(qtkit_desktop_scheme("QTKIT_TEST_THEME") == qtkit_scheme::dark, "the override first");
	check(qtkit_desktop_scheme() == qtkit_scheme::light, "an override not named is not read");
}

void test_theme_follows(QApplication &application)
{
	desktop tde("TDE");
	write(tde.trinity(), dark_kdeglobals);
	const QPalette light_palette = application.palette();

	qtkit_theme theme(application);
	int notices = 0;
	QObject::connect(&theme, &qtkit_theme::applied_changed, [&] { ++notices; });
	check(theme.applied() == qtkit_scheme::dark, "a dark desktop is applied dark");
	check(application.palette().color(QPalette::Window) ==
	          qtkit_dark_palette().color(QPalette::Window),
	      "the dark palette is on the application");
	check(application.palette().color(QPalette::Mid).lightness() <
	          application.palette().color(QPalette::WindowText).lightness(),
	      "the shading roles are dark too");

	theme.reapply();
	check(notices == 0, "nothing changed, nothing re-applied");

	// The trap: where the platform theme honours setColorScheme, Qt then
	// reports OUR answer as its hint, and without skipping it the desktop
	// going light would never be seen. Offscreen ignores the request, so
	// the trap is not exercised here; say so rather than pass silently.
	if (QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark)
		std::printf("note: this platform ignores setColorScheme; the "
		            "read-back guard is not exercised\n");
	write(tde.trinity(), light_kdeglobals);
	theme.reapply();
	check(theme.applied() == qtkit_scheme::light, "the desktop going light is followed");
	check(application.palette().color(QPalette::Window) ==
	          application.style()->standardPalette().color(QPalette::Window),
	      "light is the style's own palette, not ours");
	check(notices == 1, "one change, one notice");

	theme.set_choice(qtkit_choice::dark);
	check(theme.applied() == qtkit_scheme::dark, "chosen dark on a light desktop");
	theme.set_choice(qtkit_choice::system);
	check(theme.applied() == qtkit_scheme::light, "back to following the desktop");
	(void)light_palette;
}

// A fixed-size widget, so a size is a number the test chose.
QWidget *box(QWidget *host, int width, int height)
{
	QWidget *widget = new QWidget(host);
	widget->setFixedSize(width, height);
	return widget;
}

void test_flow_layout()
{
	QWidget host;
	auto *flow = new qtkit_flow_layout(&host, 0, 10, 10);
	QWidget *a = box(&host, 80, 20), *b = box(&host, 80, 20), *c = box(&host, 120, 30);
	flow->addWidget(a);
	flow->addWidget(b);
	flow->addWidget(c);
	check(flow->minimumSize().width() == 120, "the minimum is the widest item, not the row");
	check(flow->sizeHint() == QSize(300, 30), "the hint is the unwrapped row");
	check(flow->heightForWidth(300) == 30, "one line at the hint");
	check(flow->heightForWidth(299) == 60, "a pixel less wraps the last item");
	check(flow->heightForWidth(120) == 90, "the minimum: a line each");

	flow->setGeometry(QRect(0, 0, 200, 60));
	check(a->geometry() == QRect(0, 0, 80, 20) && b->geometry() == QRect(90, 0, 80, 20) &&
	          c->geometry() == QRect(0, 30, 120, 30),
	      "placed left to right, wrapped below the tallest of the line");

	b->hide();
	check(flow->sizeHint() == QSize(210, 30), "a hidden widget takes no room");
	flow->setGeometry(QRect(0, 0, 210, 30));
	check(c->geometry() == QRect(90, 0, 120, 30), "nor leaves a gap");
	b->show();

	// A plain widget's hint is its fixed size; one that may shrink needs a
	// hint of its own to be too wide.
	struct shrinkable : QWidget {
		using QWidget::QWidget;
		QSize sizeHint() const override { return QSize(120, 30); }
	};
	auto *wide = new shrinkable(&host);
	wide->setMinimumWidth(60);
	flow->addWidget(wide);
	flow->setGeometry(QRect(0, 0, 100, 400));
	check(wide->geometry().width() == 100, "a lone item too wide is given the line");
	flow->setGeometry(QRect(0, 0, 40, 400));
	check(wide->geometry().width() == 60, "but not below its own minimum");
	delete wide;

	host.setLayoutDirection(Qt::RightToLeft);
	flow->setGeometry(QRect(0, 0, 200, 60));
	check(a->geometry() == QRect(120, 0, 80, 20), "right to left mirrors");
}

// At the unwrapped width it lays out as a QHBoxLayout does, with the gap the
// style gives -- so a row moved onto it changes nothing on a wide window.
void test_flow_matches_a_row()
{
	QWidget flow_host, row_host;
	auto *flow = new qtkit_flow_layout(&flow_host, 0);
	auto *row = new QHBoxLayout(&row_host);
	row->setContentsMargins(0, 0, 0, 0);
	QList<QWidget *> in_flow, in_row;
	for (int width : {70, 50, 90}) {
		in_flow << box(&flow_host, width, 24);
		in_row << box(&row_host, width, 24);
		flow->addWidget(in_flow.last());
		row->addWidget(in_row.last());
	}
	check(flow->horizontal_spacing() >= 0, "the style's gap is a real number");
	check(flow->sizeHint() == row->sizeHint(), "the same hint as a row");
	const QRect natural(QPoint(0, 0), row->sizeHint());
	flow->setGeometry(natural);
	row->setGeometry(natural);
	bool same = true;
	for (int i = 0; i < in_flow.size(); ++i)
		same = same && in_flow[i]->geometry() == in_row[i]->geometry();
	check(same, "the same places as a row at the natural width");
}

void test_choice_names()
{
	check(qtkit_choice_from_name(" Dark ") == qtkit_choice::dark, "a choice read leniently");
	check(qtkit_choice_from_name("purple") == qtkit_choice::system, "an unknown word is system");
	check(qtkit_choice_name(qtkit_choice::system) == "system", "system by name");
}

}

int main(int argc, char **argv)
{
	// Before the application exists, so no platform theme answers for the
	// desktop under test.
	qputenv("QT_QPA_PLATFORMTHEME", "");
	QApplication application(argc, argv);
	test_scheme_of();
	test_files();
	test_portal_gate();
	test_order();
	test_theme_follows(application);
	test_choice_names();
	test_flow_layout();
	test_flow_matches_a_row();
	std::printf("%d check(s), %d failure(s)\n", checks, failures);
	return failures == 0 && checks > 0 ? 0 : 1;
}
