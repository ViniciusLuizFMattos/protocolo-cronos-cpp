#include "calc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
typedef long double LD;   // 64 bits de mantissa: fatoriais até 20! ficam exatos

struct Analisador {
    std::wstring t;
    size_t i = 0;
    wchar_t olhar() const { return i < t.size() ? t[i] : 0; }
    LD expr() {
        LD v = termo();
        while (olhar() == L'+' || olhar() == L'-') {
            wchar_t op = t[i++];
            LD w = termo();
            v = op == L'+' ? v + w : v - w;
        }
        return v;
    }
    LD termo() {
        LD v = potencia();
        while (olhar() == L'*' || olhar() == L'/') {
            wchar_t op = t[i++];
            LD w = potencia();
            if (op == L'/' && w == 0) throw std::runtime_error("div0");
            v = op == L'*' ? v * w : v / w;
        }
        return v;
    }
    LD potencia() {
        LD b = unario();
        if (olhar() == L'^') {
            i++;
            LD e = potencia();
            if (e != std::floor(e) || std::fabs(e) > 100) throw std::runtime_error("exp");
            return std::pow(b, e);
        }
        return b;
    }
    LD unario() {
        if (olhar() == L'-') { i++; return -unario(); }
        return pos_fixo();
    }
    LD pos_fixo() {
        LD v = atomo();
        while (olhar() == L'!') {
            i++;
            if (v != std::floor(v) || v < 0 || v > 60) throw std::runtime_error("fat");
            LD r = 1;
            for (int k = 2; k <= (int)v; k++) r *= k;
            v = r;
        }
        return v;
    }
    LD atomo() {
        if (olhar() == L'(') {
            i++;
            LD v = expr();
            if (olhar() != L')') throw std::runtime_error("par");
            i++;
            return v;
        }
        size_t ini = i;
        bool ponto = false;
        while ((olhar() >= L'0' && olhar() <= L'9') || (olhar() == L'.' && !ponto)) {
            if (olhar() == L'.') ponto = true;
            i++;
        }
        if (ini == i) throw std::runtime_error("num");
        LD v = 0, frac = 0, esc = 1;
        bool dec = false;
        for (size_t k = ini; k < i; k++) {
            if (t[k] == L'.') { dec = true; continue; }
            if (!dec) v = v * 10 + (t[k] - L'0');
            else { esc /= 10; frac += (t[k] - L'0') * esc; }
        }
        return v + frac;
    }
};
}  // namespace

bool avaliar_expressao(const std::wstring& texto, std::wstring& saida) {
    std::wstring t;
    for (wchar_t c : texto) {
        if (c == L' ') continue;
        if (c == L'×' || c == L'x' || c == L'X' || c == L'·') c = L'*';
        else if (c == L'÷') c = L'/';
        else if (c == L',') c = L'.';
        t += c;
    }
    size_t eq = t.find(L'=');
    if (eq != std::wstring::npos) t = t.substr(0, eq);
    if (t.empty()) return false;
    LD v;
    try {
        Analisador p;
        p.t = t;
        v = p.expr();
        if (p.i != t.size()) return false;
    } catch (const std::exception&) {
        return false;
    }
    if (!std::isfinite((double)v)) return false;
    char buf[64];
    LD r = std::round(v);
    if (std::fabs(v - r) <= 1e-9L * std::max((LD)1, std::fabs(v))) {
        if (std::fabs(r) < 1e15L) snprintf(buf, sizeof buf, "%.0f", (double)r);
        else snprintf(buf, sizeof buf, "%.4e", (double)r);
    } else {
        snprintf(buf, sizeof buf, "%.4f", (double)v);
        std::string s = buf;
        while (!s.empty() && s.back() == '0') s.pop_back();
        snprintf(buf, sizeof buf, "%s", s.c_str());
    }
    saida.assign(buf, buf + std::char_traits<char>::length(buf));
    return true;
}
