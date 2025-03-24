#include <iostream>
#include <string>

using namespace std;

double pamiec = 0;

double dodawanie(double a, double b) {
    double wynik = a + b;
    cout << "Dodawanie: " << a << " + " << b << " = " << wynik << endl;
    return wynik;
}

double odejmowanie(double a, double b) {
    double wynik = a - b;
    cout << "Odejmowanie: " << a << " - " << b << " = " << wynik << endl;
    return wynik;
}

double mnozenie(double a, double b) {
    double wynik = a * b;
    cout << "Mnozenie: " << a << " * " << b << " = " << wynik << endl;
    return wynik;
}

double dzielenie(double a, double b) {
    if (b == 0) {
        cout << "SYNTAX ERROR" << endl;
        return 0;
    }
    double wynik = a / b;
    cout << "Dzielenie: " << a << " / " << b << " = " << wynik << endl;
    return wynik;
}

int modulo(int a, int b) {
    if (b == 0) {
        cout << "SYNTAX ERROR" << endl;
        return 0;
    }
    int wynik = a % b;
    cout << "Modulo: " << a << " % " << b << " = " << wynik << endl;
    return wynik;
}

void mem(double wartosc) {
    cout << "Dodano " << wartosc << " do pamieci." << endl;
    pamiec = wartosc;
}

void stop() {
    cout << "Zatrzymywanie programu." << endl;
    exit(0);
}

void menu() {
    string operacja;
    cout << "==================================" << endl;
    cout << "Kalkulator" << endl;
    cout << "Dostepne opcje: " << endl;
    cout << "+ (dodawanie)" << endl;
    cout << "- (odejmowanie)" << endl;
    cout << "* (mnozenie)" << endl;
    cout << "/ (dzielenie)" << endl;
    cout << "% (modulo - tylko liczby calkowite)" << endl;
    cout << "==================================" << endl;
    cout << "Aby zakonczyc program - stop" << endl;
    cout << "Aby uzyc pamieci programu - mem" << endl;
    cout << "==================================" << endl;
    cout << "Podaj operator (+, -, *, /, %, stop): ";
    cin >> operacja;

    if (operacja == "stop") {
        stop();
    }

    if (operacja == "%") {
        int liczba1, liczba2;
        cout << "Podaj pierwsza liczbe calkowita: ";
        cin >> liczba1;
        cout << "Podaj druga liczbe calkowita: ";
        cin >> liczba2;
        mem(modulo(liczba1, liczba2));
    }
    else {
        string input1, input2;
        double liczba1, liczba2;

        cout << "Podaj pierwsza liczbe: ";
        cin >> input1;
        if (input1 == "mem") {
            liczba1 = pamiec;
        } else {
            liczba1 = stod(input1);
        }

        cout << "Podaj druga liczbe: ";
        cin >> input2;
        if (input2 == "mem") {
            liczba2 = pamiec;
        } else {
            liczba2 = stod(input2);
        }

        if (operacja == "+") {
            mem(dodawanie(liczba1, liczba2));
        } 
        else if (operacja == "-") {
            mem(odejmowanie(liczba1, liczba2));
        } 
        else if (operacja == "*") {
            mem(mnozenie(liczba1, liczba2));
        } 
        else if (operacja == "/") {
            mem(dzielenie(liczba1, liczba2));
        } 
        else {
            cout << "Niepoprawny operator." << endl;
        }
    }
}

int main() {
    while (true) {
        menu();
    }
    return 0;
}
