/***************************************************************************
 *   Copyright (C) 2011 by Renaud Guezennec                                *
 *   http://renaudguezennec.homelinux.org/accueil,3.html                   *
 *                                                                         *
 *   Rolisteam is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>

#include "die.h"
#include "diceparser.h"

class TestDice : public QObject
{
    Q_OBJECT

public:
    TestDice();

private slots:
    void initTestCase();
    void getAndSetTest();
    void diceRollD10Test();
    void diceRollD20Test();
    void commandEndlessLoop();
    void mathPriority();
    void commandsTest();
    void wrongCommandsTest();
    void wrongCommandsExecutionTimeTest();
    void scopeDF();
    void cleanupTestCase();

private:
    Die* m_die;
    DiceParser* m_diceParser;
};

TestDice::TestDice()
{
}

void TestDice::initTestCase()
{
    m_die = new Die();
    m_diceParser = new DiceParser();
}

void TestDice::getAndSetTest()
{
    for(unsigned int i = 0; i<2000;i++)
    {
        m_die->setMaxValue(i);
        QVERIFY(m_die->getMaxValue()==i);
    }

    m_die->setSelected(true);
    QVERIFY(m_die->isSelected()==true);

    m_die->setSelected(false);
    QVERIFY(m_die->isSelected()==false);

}

void TestDice::diceRollD10Test()
{
    m_die->setMaxValue(10);
    for(int i = 0; i< 2000; i++)
    {
        m_die->roll(false);
        QVERIFY(m_die->getValue()>0);
        QVERIFY(m_die->getValue()<11);
    }

}
void TestDice::diceRollD20Test()
{
    m_die->setMaxValue(20);
    for(int i = 0; i< 2000; i++)
    {
        m_die->roll(false);
        QVERIFY(m_die->getValue()>0);
        QVERIFY(m_die->getValue()<21);
    }
}
void TestDice::commandEndlessLoop()
{
    bool a = m_diceParser->parseLine("1D10e[>0]");
    QVERIFY(a==false);
}

void TestDice::commandsTest()
{
    QStringList commands;

   commands << "1L[cheminée,chocolat,épée,arc,chute de pierre]"
            << "10d10c[>=6]-@c[=1]"
            << "10d10c[>=6]-@c[=1]-@c[=1]"
            << "10d10c[>6]+@c[=10]"
            << "1+1D10"
            << "3d10c[>=5]"
            << "3nwod"
            << "1+(4*3)D10"
            << "2+4/4"
            << "2D10*2D20*8"
            <<"1+(4*3)D10"
            <<"(4D6)D10"
            << "1D100a[>=95]a[>=96]a[>=97]a[>=98]a[>=99]e[>=100]"
            << "3D100"
            << "4k3"
            << "10D10e[>=6]sc[>=6]"
            << "100190D6666666s"
            << "10D10e10s"
            << "10D10s"
            << "15D10e10c[8-10]"
            << "10d10e11"
            << "1D8+2D6+7"
            << "D25"
            << "1L[tete[10],ventre[50],jambe[40]]"
            << "2d6c[%2=0]"
            << "D25+D10"
            << "D25;D10"
            << "8+8+8"
            << "1D20-88"
            << "100*1D20*2D6"
            << "100/28*3"
            << "100/8"
            << "100*3*8"
            << "help"
            << "la"
            << "10D10c[<2|>7]"
            << "1L[tete,bras droit,bras gauche,jambe droite,jambe gauche,ventre[6-7],buste[8-10]]"
            << "10D6c[=2|=4|=6]"
            <<"10D10e[=1|=10]k4"
            << "10+10s"
            << "1d6e6;1d4e4mk1"
            << "1d6e6;1d4e4mk1"
            << "400000D20/400000"
            << "1d100e[>=95]i[<5]{-1d100e95}"
            << "100*3*8"
            << "1d100i[<70]{1d10i[=10]{1d100i[<70]{1d10e10}}}"
            << "10d6c[<2|>5]"
            << "5-5*5+5"
            << "4d6i[=6]{+1d6}"
            << "10d[-8--1]"
            << "4d6e6i[=4]{-4}+2"
            << "4d6e6f[!=4]+2"
            << "5d10g10"
            << "4d6p[4:blue]c[>=4];1d6p[1:#FFFFFF]c6-@c1;1d6p[1:#FF0000]c[>=4]+@c6-@c1"
            << "10d[0-9]";
    for(QString cmd : commands)
    {
        qDebug() << cmd;
        bool a = m_diceParser->parseLine(cmd);
        QVERIFY2(a==true,cmd.toStdString().c_str());

        m_diceParser->start();
        if(!m_diceParser->humanReadableError().isEmpty())
            qDebug() << m_diceParser->humanReadableError();
        QVERIFY2(m_diceParser->humanReadableError().isEmpty(),cmd.toStdString().c_str());
    }
}
void TestDice::wrongCommandsTest()
{
    QStringList commands;

    commands << "1L[cheminée,chocolat,épée,arc,chute de pierre"
            << "10d10c"
            << "10d10a"
            << "10d0a[>7]"
            << "aiteanetauearuteurn"
            << "pajaejlbnmàw";

    foreach(QString cmd, commands)
    {
        bool a = m_diceParser->parseLine(cmd);

        QVERIFY2(a==false,cmd.toStdString().c_str());
    }
}
void TestDice::wrongCommandsExecutionTimeTest()
{
    QStringList commands;

    //<< "8D20+10*@c[=20]"
    commands << "1/0"
             << ""
             << "0d10"
             << "10d10k11"
             << "!!!!";

    foreach(QString cmd, commands)
    {
        bool test = m_diceParser->parseLine(cmd);
        m_diceParser->start();

        QVERIFY2(m_diceParser->getErrorMap().isEmpty() == false || !test,cmd.toStdString().c_str());
    }
}
void TestDice::scopeDF()
{
    QStringList commands;

    commands << "1D[-1-1]"
             << "1D[-10--5]"
             << "1D[-100-100]"
             << "1D[-1-0]"
             << "1D[10-20]"
             << "1D[30-100]"
             << "1D[0-99]"
             << "5-5*5+5"
             << "2d20c[<=13]+@c[<=3]"
             << "6d10c[>=6]-@c1"
             << "1D[-2-50]";

    QList< QPair<int,int> > pairMinMax;
    pairMinMax  << QPair<int,int>(-1,1)
                << QPair<int,int>(-10,-5)
                << QPair<int,int>(-100,100)
                << QPair<int,int>(-1,0)
                << QPair<int,int>(10,20)
                << QPair<int,int>(30,100)
                << QPair<int,int>(0,99)
                << QPair<int,int>(-15,-15)
                << QPair<int,int>(0,4)
                << QPair<int,int>(-6,6)
                << QPair<int,int>(-2,50);


    int index = 0;
    for(QString cmd: commands)
    {
        bool test = m_diceParser->parseLine(cmd);
        QVERIFY2(test,cmd.toStdString().c_str());
        m_diceParser->start();
        QPair<int,int> expect = pairMinMax.at(index);
        int min = expect.first;
        int max = expect.second;
        auto  results = m_diceParser->getLastIntegerResults();

        for(auto result: results)
            QVERIFY2(result >= min && result <= max,cmd.toStdString().c_str());
        index++;
    }
}
void TestDice::mathPriority()
{
    QStringList commands;

    commands << "10+2"
             << "2-10"
             << "5+2*3"
             << "5-5*5+5"
             << "5-5/5+5"
             << "10*(3*2)"
             << "60/(3*2)"
             << "5-(5*5+5)";

    QList<qreal> results;
    results <<12 <<-8 << 11 << -15 << 9 << 60 << 10 << -25;


    for(QString cmd:  commands)
    {
        bool test = m_diceParser->parseLine(cmd);
        QVERIFY2(test,cmd.toStdString().c_str());
        m_diceParser->start();
        int index = commands.indexOf(cmd);
        int expect = results.at(index);
        auto resultList = m_diceParser->getLastIntegerResults();
        qDebug() << resultList.size() << expect << test << resultList;

        for(auto result: resultList)
            QVERIFY2(result == expect,cmd.toStdString().c_str());
    }
}

void TestDice::cleanupTestCase()
{
    delete m_die;
}

QTEST_MAIN(TestDice);

#include "tst_dice.moc"
