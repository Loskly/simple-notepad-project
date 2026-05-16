#include "danger_mode.h"
#include <QAction>
#include <QInputDialog>
#include <QMenuBar>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTextCharFormat>

danger_mode::danger_mode(QTextEdit *editor, QMainWindow *parent)
    : QObject(parent)
      , editor(editor)
      , parent_window(parent) {
    deletion_timer = new QTimer(this);
    deletion_timer->setSingleShot(true);
    connect(deletion_timer, &QTimer::timeout, this, &danger_mode::delete_unsafe_text);

    countdown_timer = new QTimer(this);
    countdown_timer->setInterval(100);
    connect(countdown_timer, &QTimer::timeout, this, &danger_mode::update_countdown);
    countdown_timer->start();

    connect(editor, &QTextEdit::textChanged, this, &danger_mode::reset_deletion_timer);
}

void danger_mode::setup_menu(QMenuBar *menu_bar) {
    auto *danger_menu = menu_bar->addMenu("Danger Mode");

    auto *action_interval = danger_menu->addAction("Interval of Word Deletion...");
    connect(action_interval, &QAction::triggered, this, [this] {
        show_interval_dialog();
    });

    auto *action_checkpoint = danger_menu->addAction("Set Checkpoint Interval...");
    connect(action_checkpoint, &QAction::triggered, this, [this] {
        show_checkpoint_dialog();
    });
}

void danger_mode::setup_status_bar(QStatusBar *status_bar) {
    countdown_label = new QLabel(parent_window);
    status_bar->addPermanentWidget(countdown_label);
}

void danger_mode::reset() {
    safe_position = 0;
    next_checkpoint = checkpoint_interval;
}

int danger_mode::current_word_count() const {
    const QString text = editor->toPlainText();
    return text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).length();
}

void danger_mode::delete_unsafe_text() {
    const QString text = editor->toPlainText();
    if (text.length() <= safe_position) {
        return;
    }

    auto cursor = editor->textCursor();
    cursor.setPosition(safe_position);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    next_checkpoint = current_word_count() + checkpoint_interval;
    update_protected_highlight();
}

void danger_mode::reset_deletion_timer() {
    const int text_len = editor->toPlainText().length();

    if (safe_position > text_len) {
        safe_position = text_len;
    }

    const int words = current_word_count();

    if (words >= next_checkpoint) {
        safe_position = text_len;
        next_checkpoint = words + checkpoint_interval;
        deletion_timer->stop();
        update_protected_highlight();
        return;
    }

    if (text_len > safe_position) {
        deletion_timer->start(deletion_interval_sec * 1000);
    } else {
        deletion_timer->stop();
    }

    update_protected_highlight();
}

void danger_mode::show_interval_dialog() {
    bool ok = false;
    const int value = QInputDialog::getInt(
        parent_window,
        "Word Deletion Interval",
        "Seconds before unprotected text is deleted:",
        deletion_interval_sec,
        1, // min
        60, // max
        1, // step
        &ok);
    if (ok) {
        deletion_interval_sec = value;
        reset_deletion_timer();
    }
}

void danger_mode::show_checkpoint_dialog() {
    bool ok = false;
    const int value = QInputDialog::getInt(
        parent_window,
        "Checkpoint Interval",
        "Number of words between checkpoints:",
        checkpoint_interval,
        10, // min
        500, // max
        10, // step
        &ok);
    if (ok) {
        checkpoint_interval = value;
        const int words = current_word_count();
        next_checkpoint = ((words / checkpoint_interval) + 1) * checkpoint_interval;
        reset_deletion_timer();
    }
}

void danger_mode::update_protected_highlight() {
    editor->blockSignals(true);

    const auto saved_cursor = editor->textCursor();
    const int text_len = editor->toPlainText().length();

    QTextCharFormat highlight_fmt;
    highlight_fmt.setBackground(QColor(144, 238, 144, 50));

    QTextCharFormat normal_fmt;
    normal_fmt.setBackground(Qt::transparent);

    if (safe_position > 0) {
        auto cursor = editor->textCursor();
        cursor.setPosition(0);
        cursor.setPosition(safe_position, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(highlight_fmt);
    }

    if (safe_position < text_len) {
        auto cursor = editor->textCursor();
        cursor.setPosition(safe_position);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(normal_fmt);
    }

    editor->setTextCursor(saved_cursor);
    editor->blockSignals(false);
}

void danger_mode::update_countdown() {
    if (!countdown_label) return;

    QString timer_text;
    if (deletion_timer->isActive()) {
        const int remaining_ms = deletion_timer->remainingTime();
        const double remaining_sec = remaining_ms / 1000.0;
        timer_text = "Timer: " + QString::number(remaining_sec, 'f', 1) + "s";
        countdown_label->setStyleSheet("color: red; font-weight: bold;");
    } else {
        timer_text = "Timer: Safe";
        countdown_label->setStyleSheet("color: green; font-weight: bold;");
    }

    countdown_label->setText("  " + timer_text);
}
