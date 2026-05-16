#ifndef DANGER_MODE_H
#define DANGER_MODE_H

#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QTextEdit>
#include <QTimer>

class danger_mode : public QObject {
public:
    danger_mode(QTextEdit* editor, QMainWindow* parent);

    void setup_menu(QMenuBar* menu_bar);
    void setup_status_bar(QStatusBar* status_bar);
    void reset();

private:
    void delete_unsafe_text();
    void reset_deletion_timer();
    void update_protected_highlight();
    void update_countdown();
    void show_interval_dialog();
    void show_checkpoint_dialog();

    int current_word_count() const;

    QTextEdit* editor;
    QMainWindow* parent_window;

    QTimer* deletion_timer { nullptr };
    QTimer* countdown_timer { nullptr };
    QLabel* countdown_label { nullptr };

    int deletion_interval_sec { 5 };
    int checkpoint_interval { 20 };
    int next_checkpoint { 20 };
    int safe_position { 0 };
};

#endif // DANGER_MODE_H
