/*************************************************************************
 *     Copyright (C) 2011 by Joseph Boudou                               *
 *                                                                       *
 *     http://www.rolisteam.org/                                         *
 *                                                                       *
 *   Rolisteam is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published   *
 *   by the Free Software Foundation; either version 2 of the License,   *
 *   or (at your option) any later version.                              *
 *                                                                       *
 *   This program is distributed in the hope that it will be useful,     *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 *   GNU General Public License for more details.                        *
 *                                                                       *
 *   You should have received a copy of the GNU General Public License   *
 *   along with this program; if not, write to the                       *
 *   Free Software Foundation, Inc.,                                     *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           *
 *************************************************************************/


#include <QTreeView>
#include <QVBoxLayout>

#include "playersListWidget.h"

#include "Carte.h"
#include "persons.h"
#include "persondialog.h"
#include "playersList.h"


/**************************
 * PlayersListWidgetModel *
 **************************/

PlayersListWidgetModel::PlayersListWidgetModel(QObject * parent)
    : QAbstractProxyModel(parent), m_map(NULL)
{
    PlayersList & g_playersList = PlayersList::instance();
    setSourceModel(&g_playersList);

    // Proxy mecanic
    connect(&g_playersList, SIGNAL(rowsAboutToBeInserted(const QModelIndex &,int,int)),
            this, SLOT(p_rowsAboutToBeInserted(const QModelIndex &,int,int)));
    connect(&g_playersList, SIGNAL(rowsInserted(const QModelIndex &,int,int)),
            this, SLOT(p_rowsInserted()));
    connect(&g_playersList, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &,int,int)),
            this, SLOT(p_rowsAboutToBeRemoved(const QModelIndex &,int,int)));
    connect(&g_playersList, SIGNAL(rowsRemoved(const QModelIndex &,int,int)),
            this, SLOT(p_rowsRemoved()));
    connect(&g_playersList, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(p_dataChanged(const QModelIndex &, const QModelIndex &)));
}

QModelIndex PlayersListWidgetModel::mapFromSource(const QModelIndex & sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();

    quint32 parentRow = (quint32)(sourceIndex.internalId() & PlayersList::NoParent);
    return createIndex(sourceIndex.row(), sourceIndex.column(), parentRow);
}

QModelIndex PlayersListWidgetModel::mapToSource(const QModelIndex & proxyIndex) const
{
    return PlayersList::instance().mapIndexToMe(proxyIndex);
}

QModelIndex PlayersListWidgetModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex sourceIndex = sourceModel()->index(row, column, mapToSource(parent));
    return mapFromSource(sourceIndex);
}

QModelIndex PlayersListWidgetModel::parent(const QModelIndex &index) const
{
    QModelIndex sourceIndex = sourceModel()->parent(mapToSource(index));
    return mapFromSource(sourceIndex);
}

int PlayersListWidgetModel::rowCount(const QModelIndex &parent) const
{
    return sourceModel()->rowCount(mapToSource(parent));
}

int PlayersListWidgetModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

Qt::ItemFlags PlayersListWidgetModel::flags(const QModelIndex &index) const
{
    if (isCheckable(index))
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable |
               Qt::ItemIsUserCheckable;

    if (index.parent().isValid())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled;
}

QVariant PlayersListWidgetModel::data(const QModelIndex &index, int role) const
{
    if (isCheckable(index) && role == Qt::CheckStateRole)
    {
        return QVariant(m_map->pjAffiche(PlayersList::instance().getPerson(index)->uuid()));
    }

    return QAbstractProxyModel::data(index, role);
}

bool PlayersListWidgetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (isCheckable(index) && role == Qt::CheckStateRole)
    {
        // isCheckable ensures person and m_map is not NULL and person is a character.
        m_map->toggleCharacterView(static_cast<Character *>(PlayersList::instance().getPerson(index)));
        emit dataChanged(index, index);
        return true;
    }

    return QAbstractProxyModel::setData(index, value, role);
}

void PlayersListWidgetModel::changeMap(Carte * map)
{
    if (map == m_map)
        return;

    m_map = map;

    // We need to tell which rows should be updated

    PlayersList & g_playersList = PlayersList::instance();
    QModelIndex begin;
    QModelIndex end;
    int i;
    int max = (g_playersList.localPlayer()->isGM() ? g_playersList.numPlayers() : 1);

    for (i = 0 ; i < max ; i++)
    {
        Player * player = g_playersList.getPlayer(i);
        int nbCharacters = player->getCharactersCount();

        if (nbCharacters > 0)
        {
            begin = createIndex(0, 0, i);
            end   = createIndex(nbCharacters, 0, i);
            break;
        }
    }
    for (; i < max ; i++)
    {
        Player * player = g_playersList.getPlayer(i);
        int nbCharacters = player->getCharactersCount();

        if (nbCharacters > 0)
            end   = createIndex(nbCharacters, 0, i);
    }

    if (begin.isValid() && end.isValid())
        emit dataChanged(begin, end);
}

bool PlayersListWidgetModel::isCheckable(const QModelIndex &index) const
{
    if (!index.isValid() || m_map == NULL)
        return false;

    PlayersList & g_playersList = PlayersList::instance();

    Person * person = g_playersList.getPerson(index);
    if (person == NULL)
        return false;

    Player * localPlayer = g_playersList.localPlayer();

    return ((person->parent() == localPlayer) ||
            (localPlayer->isGM() && index.parent().isValid()));
}

void PlayersListWidgetModel::p_rowsAboutToBeInserted(const QModelIndex & parent, int start, int end)
{
    beginInsertRows(mapFromSource(parent), start, end);
}

void PlayersListWidgetModel::p_rowsInserted()
{
    endInsertRows();
}

void PlayersListWidgetModel::p_rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
    beginRemoveRows(mapFromSource(parent), start, end);
}

void PlayersListWidgetModel::p_rowsRemoved()
{
    endRemoveRows();
}

void PlayersListWidgetModel::p_dataChanged(const QModelIndex & from, const QModelIndex & to)
{
    emit dataChanged(mapFromSource(from), mapFromSource(to));
}


/********************
 * PlayerListWidget *
 ********************/

PlayersListWidget::PlayersListWidget(QWidget * parent)
    : QDockWidget(parent)
{
    setAllowedAreas(Qt::AllDockWidgetAreas);
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    setWindowTitle(tr("Joueurs"));

    setUI();
}

PlayersListWidgetModel * PlayersListWidget::model() const
{
    return m_model;
}

void PlayersListWidget::editIndex(const QModelIndex & index)
{
    if (!index.isValid())
        return;

    PlayersList & g_playersList = PlayersList::instance();
    Person * person = g_playersList.getPerson(index);
    if (!g_playersList.isLocal(person))
        return;

    if (m_personDialog->edit(tr("Editer"), person->name(), person->color()) == QDialog::Accepted)
    {
        g_playersList.changeLocalPerson(person, m_personDialog->getName(), m_personDialog->getColor());
    }
}

void PlayersListWidget::createLocalCharacter()
{
    PlayersList & g_playersList = PlayersList::instance();
    Player * localPlayer = g_playersList.localPlayer();

    if (m_personDialog->edit(tr("Nouveau personnage"), tr("Nouveau personnage"), localPlayer->color()) == QDialog::Accepted)
    {
        g_playersList.addLocalCharacter(new Character(m_personDialog->getName(), m_personDialog->getColor()));
    }
}


void PlayersListWidget::selectAnotherPerson(const QModelIndex & current)
{
    PlayersList & g_playersList = PlayersList::instance();
    m_delButton->setEnabled(current.isValid() && current.parent().isValid() &&
            g_playersList.isLocal(g_playersList.getPerson(current)));
}


void PlayersListWidget::deleteSelected()
{
    PlayersList & g_playersList = PlayersList::instance();
    QModelIndex current = m_selectionModel->currentIndex();
    if (current.isValid() && current.parent().isValid() &&
            g_playersList.isLocal(g_playersList.getPerson(current)))
    {
        g_playersList.delLocalCharacter(current.row());
        m_delButton->setEnabled(false);
    }
}

void PlayersListWidget::setUI()
{
    // Central Widget
    QWidget * centralWidget = new QWidget(this);

    // PlayersListView
    QTreeView * playersListView  = new QTreeView(centralWidget);
    m_model = new PlayersListWidgetModel;
    playersListView->setModel(m_model);
    m_selectionModel = playersListView->selectionModel();
    playersListView->setHeaderHidden(true);

    // Add PJ button
    QPushButton * addPlayerButton = new QPushButton(tr("Ajouter un PJ"), centralWidget);

    // Del PJ buttun
    m_delButton = new QPushButton(tr("Supprimer un PJ"), centralWidget);
    m_delButton->setEnabled(false);

    // Button layout
    QHBoxLayout * buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addPlayerButton);
    buttonLayout->addWidget(m_delButton);

    // Layout
    QVBoxLayout * layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 3, 3);
    layout->addWidget(playersListView);
    layout->addLayout(buttonLayout);
    centralWidget->setLayout(layout);
    setWidget(centralWidget);

    // Actions
    connect(playersListView, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(editIndex(const QModelIndex &)));
    connect(m_selectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(selectAnotherPerson(const QModelIndex &)));
    connect(m_model, SIGNAL(rowsRemoved( const QModelIndex &, int, int)),
            playersListView, SLOT(clearSelection()));
    connect(addPlayerButton, SIGNAL(pressed()), this, SLOT(createLocalCharacter()));
    connect(m_delButton, SIGNAL(pressed()), this, SLOT(deleteSelected()));

    // Dialog
    m_personDialog = new PersonDialog(this);
}
