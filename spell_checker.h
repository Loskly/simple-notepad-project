#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <QSyntaxHighlighter>
#include <QTextEdit>
#include <unordered_set>
#include <string>
#include <vector>
#include <QString>
#include <QPoint>

class spell_checker : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit spell_checker(QTextDocument *parent = nullptr);
    void load_dictionary(const QString& path);
    bool is_correct(const QString& word) const;
    std::vector<QString> get_suggestions(const QString& word) const;
    void attach_to_editor(QTextEdit* editor);
    void check_spelling();

protected:
    void highlightBlock(const QString &text) override;

private slots:
    void show_context_menu(const QPoint& pt);

private:
    std::unordered_set<std::string> dictionary;
    QTextEdit* text_editor { nullptr };
};

#endif // SPELL_CHECKER_H
