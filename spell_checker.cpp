#include "spell_checker.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMenu>
#include <QTextCursor>
#include <QAction>
#include <algorithm>
#include <cmath>

spell_checker::spell_checker(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
}

void spell_checker::load_dictionary(const QString& path)
{
    QStringList paths = {
        path,
        "../" + path,
        "../../" + path,
        "../../../" + path
    };
    QFile file;
    bool opened = false;
    for (const auto& p : paths) {
        file.setFileName(p);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            opened = true;
            break;
        }
    }
    if (!opened) return;
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QString clean;
        for (QChar c : line) {
            if (c.isLetter()) {
                clean += c.toLower();
            }
        }
        if (!clean.isEmpty()) {
            dictionary.insert(clean.toStdString());
        }
    }
}

bool spell_checker::is_correct(const QString& word) const
{
    if (dictionary.empty()) return true;
    
    QString clean_word;
    for (QChar c : word) {
        if (c.isLetter()) {
            clean_word += c.toLower();
        }
    }
    
    if (clean_word.isEmpty()) return true;
    
    return dictionary.find(clean_word.toStdString()) != dictionary.end();
}

void spell_checker::highlightBlock(const QString &text)
{
    if (dictionary.empty()) return;

    QTextCharFormat error_format;
    error_format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    error_format.setUnderlineColor(Qt::red);

    QRegularExpression word_regex("\\b[a-zA-Z]+\\b");
    QRegularExpressionMatchIterator i = word_regex.globalMatch(text);
    
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured();
        if (!is_correct(word)) {
            setFormat(match.capturedStart(), match.capturedLength(), error_format);
        }
    }
}

static int edit_distance(const std::string& s1, const std::string& s2, int max_dist) {
    const size_t m = s1.size();
    const size_t n = s2.size();
    if (std::abs((int)m - (int)n) > max_dist) return max_dist + 1;
    
    std::vector<int> prev(n + 1);
    std::vector<int> curr(n + 1);
    
    for (size_t j = 0; j <= n; ++j) prev[j] = j;
    
    for (size_t i = 1; i <= m; ++i) {
        curr[0] = i;
        int min_val = curr[0];
        for (size_t j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            curr[j] = std::min({ curr[j - 1] + 1, prev[j] + 1, prev[j - 1] + cost });
            min_val = std::min(min_val, curr[j]);
        }
        if (min_val > max_dist) return max_dist + 1; // Early exit
        prev = curr;
    }
    return prev[n];
}

std::vector<QString> spell_checker::get_suggestions(const QString& word) const
{
    QString clean_word;
    for (QChar c : word) {
        if (c.isLetter()) {
            clean_word += c.toLower();
        }
    }
    std::string target = clean_word.toStdString();
    
    std::vector<std::pair<int, std::string>> candidates;
    for (const auto& dict_word : dictionary) {
        if (std::abs((int)dict_word.size() - (int)target.size()) > 2) continue;
        int dist = edit_distance(target, dict_word, 2);
        if (dist <= 2) {
            candidates.push_back({dist, dict_word});
        }
    }
    
    std::sort(candidates.begin(), candidates.end());
    
    std::vector<QString> results;
    for (size_t i = 0; i < std::min(size_t(5), candidates.size()); ++i) {
        results.push_back(QString::fromStdString(candidates[i].second));
    }
    return results;
}

void spell_checker::attach_to_editor(QTextEdit* editor)
{
    text_editor = editor;
    setDocument(editor->document());
    
    editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editor, &QTextEdit::customContextMenuRequested, this, &spell_checker::show_context_menu);
}

void spell_checker::check_spelling()
{
    rehighlight();
}

void spell_checker::show_context_menu(const QPoint& pt)
{
    if (!text_editor) return;
    
    QTextCursor cursor = text_editor->cursorForPosition(pt);
    cursor.select(QTextCursor::WordUnderCursor);
    QString word = cursor.selectedText();
    
    QMenu* menu = text_editor->createStandardContextMenu();
    
    if (!word.isEmpty() && !is_correct(word)) {
        std::vector<QString> suggestions = get_suggestions(word);
        if (!suggestions.empty()) {
            menu->insertSeparator(menu->actions().first());
            
            for (auto it = suggestions.rbegin(); it != suggestions.rend(); ++it) {
                QString sug = *it;
                QAction* action = new QAction(sug, menu);
                connect(action, &QAction::triggered, [this, cursor, sug]() mutable {
                    cursor.insertText(sug);
                });
                menu->insertAction(menu->actions().first(), action);
            }
        }
    }
    
    menu->exec(text_editor->mapToGlobal(pt));
    delete menu;
}
