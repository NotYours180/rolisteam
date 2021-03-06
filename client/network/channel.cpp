#include "channel.h"
#include "tcpclient.h"

Channel::Channel()
{

}

Channel::Channel(QString str)
{
    m_name = str;
}

Channel::~Channel()
{

}

QByteArray Channel::password() const
{
    return m_password;
}

void Channel::setPassword(const QByteArray &password)
{
    m_password = password;
}

int Channel::childCount() const
{
    return m_child.size();
}

int Channel::indexOf(TreeItem *child)
{
    return m_child.indexOf(child);
}

QString Channel::description() const
{
    return m_description;
}

void Channel::setDescription(const QString &description)
{
    m_description = description;
}

bool Channel::usersListed() const
{
    return m_usersListed;
}

void Channel::setUsersListed(bool usersListed)
{
    m_usersListed = usersListed;
}

bool Channel::isLeaf() const
{
    return false;
}
void Channel::readFromJson(QJsonObject &json)
{
    m_password=QByteArray::fromBase64(json["password"].toString().toUtf8());
    m_name=json["title"].toString();
    m_description=json["description"].toString();
    m_usersListed=json["usersListed"].toBool();
    m_id=json["id"].toString();

    QJsonArray array = json["children"].toArray();
    for(auto channelJson : array)
    {
        QJsonObject obj = channelJson.toObject();
        TreeItem* item = nullptr;
        if(obj["type"]=="channel")
        {
            Channel* chan = new Channel();
            item = chan;
        }
        else
        {
            TcpClient* tcpItem = new TcpClient(nullptr,nullptr);
            item = tcpItem;
            qDebug() << "gm status" << tcpItem << obj["gm"].toBool();
            if(obj["gm"].toBool())
            {
                m_currentGm = tcpItem;
            }
        }
        item->readFromJson(obj);
        item->setParentItem(this);
        m_child.append(item);
    }
}

void Channel::writeIntoJson(QJsonObject &json)
{
    json["password"]=QString::fromUtf8(m_password.toBase64());
    json["title"]=m_name;
    json["description"]=m_description;
    json["usersListed"]=m_usersListed;
    json["id"]=m_id;
    json["type"]="channel";

    QJsonArray array;
    for (int i = 0; i < m_child.size(); ++i)
    {
        if(m_child.at(i)->isLeaf())
        {
            TreeItem* item = m_child.at(i);
           // Channel* item = dynamic_cast<Channel*>(m_child.at(i));
            if(nullptr != item)
            {
                QJsonObject jsonObj;
                item->writeIntoJson(jsonObj);
                jsonObj["gm"]=(item == m_currentGm);
                array.append(jsonObj);
            }
        }
    }
    json["children"]=array;
}

TreeItem *Channel::getChildAt(int row)
{
    if(m_child.isEmpty()) return nullptr;
    if(m_child.size() > row)
    {
        return m_child.at(row);
    }
    return nullptr;
}
void Channel::sendMessage(NetworkMessage* msg, TcpClient* emitter, bool mustBeSaved)
{  
    if(msg->getRecipientMode() == NetworkMessage::All)
    {
        sendToAll(msg,emitter);
        if(mustBeSaved)
        {
            m_dataToSend.append(msg);
            m_memorySize += msg->getSize();
            emit memorySizeChanged(m_memorySize,this);
        }
    }
    else if(msg->getRecipientMode() == NetworkMessage::OneOrMany) 
    {
        sendToMany(msg,emitter);
    }

}
void Channel::sendToMany(NetworkMessage* msg, TcpClient* tcp)
{ 
    auto const recipient = msg->getRecipientList();
    for(auto client : m_child)
    {
        TcpClient* other = dynamic_cast<TcpClient*>(client);

        if((nullptr != other)&&(other!=tcp)&&(recipient.contains(other->getId())))
        {
            QMetaObject::invokeMethod(other,"sendMessage",Qt::QueuedConnection,Q_ARG(NetworkMessage*,msg),Q_ARG(bool,false));
        }          
    }
}

void Channel::sendToAll(NetworkMessage* msg, TcpClient* tcp)
{
    for(auto client : m_child)
    {
        TcpClient* other = dynamic_cast<TcpClient*>(client);
        if((nullptr != other)&&(other!=tcp))
        {
            QMetaObject::invokeMethod(other,"sendMessage",Qt::QueuedConnection,Q_ARG(NetworkMessage*,msg),Q_ARG(bool,false));
        }          
    }
}

int Channel::addChild(TreeItem* item)
{
    if(nullptr != item)
    {
        m_child.append(item);
        item->setParentItem(this);
        TcpClient* tcp = dynamic_cast<TcpClient*>(item);
        if(nullptr!=tcp)
        {
            if(tcp->isGM())
            {
                sendOffGmStatus(tcp);
            }
            updateNewClient(tcp);
        }
        return m_child.size();
    }
    return -1;

}

bool Channel::addChildInto(QString id, TreeItem* child)
{
    if(m_id == id)
    {
        addChild(child);
        return true;
    }
    else
    {
        for(TreeItem* item : m_child)
        {
            if(item->addChildInto(id, child))
            {
                return true;
            }
        }
    }
    return false;
}

void Channel::updateNewClient(TcpClient* newComer)
{
    NetworkMessageWriter* msg1 = new NetworkMessageWriter(NetMsg::AdministrationCategory,NetMsg::ClearTable);
    msg1->string8(newComer->getId());
    QMetaObject::invokeMethod(newComer,"sendMessage",Qt::QueuedConnection,Q_ARG(NetworkMessage*,msg1),Q_ARG(bool,true));
    //Sending players infos
    for(auto child : m_child)
    {
        if(child->isLeaf())
        {
            TcpClient* tcpConnection = dynamic_cast<TcpClient*>(child);
            if(nullptr != tcpConnection)
            {
                if((tcpConnection != newComer)&&(tcpConnection->isFullyDefined()))
                {
                    NetworkMessageWriter* msg = new NetworkMessageWriter(NetMsg::PlayerCategory,NetMsg::PlayerConnectionAction);
                    tcpConnection->fill(msg);
                    QMetaObject::invokeMethod(newComer,"sendMessage",Qt::QueuedConnection,Q_ARG(NetworkMessage*,msg),Q_ARG(bool,true));

                    NetworkMessageWriter* msg2 = new NetworkMessageWriter(NetMsg::PlayerCategory,NetMsg::PlayerConnectionAction);
                    newComer->fill(msg2);
                    QMetaObject::invokeMethod(tcpConnection,"sendMessage",Qt::QueuedConnection,Q_ARG(NetworkMessage*,msg2),Q_ARG(bool,true));
                }
            }
        }
    }
    //resend all previous data received
    for(auto msg : m_dataToSend)
    {
        //tcp->sendMessage(msg);
        QMetaObject::invokeMethod(newComer,"sendMessage",Qt::QueuedConnection,Q_ARG(NetworkMessage*,msg),Q_ARG(bool,false));
    }
}

void Channel::kick(QString str)
{
    bool found = false;
    for(TreeItem* item : m_child)
    {
        if(item->getId() == str)
        {
            //child = item;
            found = true;
            m_child.removeAll(item);
            emit itemChanged();

            TcpClient* client = dynamic_cast<TcpClient*>(item);
            removeClient(client);
            QMetaObject::invokeMethod(client,"closeConnection",Qt::QueuedConnection);
        }
    }
    if(!found)
    {
        for(TreeItem* item : m_child)
        {
            item->kick(str);
        }
    }
}



void Channel::clear()
{
    for(TreeItem* item : m_child)
    {
        item->clear();
    }
    qDeleteAll(m_child);
    m_child.clear();
}
TreeItem* Channel::getChildById(QString id)
{
    for(TreeItem* item : m_child)
    {
        if(item->getId() == id)
        {
            return item;
        }
        else
        {
            auto itemChild = item->getChildById(id);
            if(nullptr != itemChild)
            {
                return itemChild;
            }
        }
    }
    return nullptr;
}

TcpClient* Channel::getPlayerById(QString id)
{
    for(auto item : m_child)
    {
        if(nullptr == item)
            continue;

        auto client = dynamic_cast<TcpClient*>(item);
        if(client)
        {
            if(client->getPlayerId() == id)
            {
                return client;
            }
        }
        else
        {
            auto channel = dynamic_cast<Channel*>(item);
            if(nullptr!= channel)
            {
                return channel->getPlayerById(id);
            }
        }
    }
    return nullptr;
}

void Channel::insertChildAt(int pos, TreeItem* item)
{
    m_child.insert(pos,item);
}

void Channel::fill(NetworkMessageWriter& msg)
{
    msg.string8(m_id);
    msg.string16(m_name);
    msg.string16(m_description);
}
void Channel::read(NetworkMessageReader& msg)
{
    m_id= msg.string8();
    m_name = msg.string16();
    m_description = msg.string16();
}

bool Channel::removeClient(TcpClient* client)
{
    //must be the first line
    int i = m_child.removeAll(client);

    //notify all remaining chan member to remove former player
    NetworkMessageWriter* message = new NetworkMessageWriter(NetMsg::PlayerCategory, NetMsg::DelPlayerAction);
    message->string8(client->getPlayerId());
    sendMessage(message,nullptr,false);

    if(hasNoClient())
    {
        clearData();
    }
    else if((m_currentGm != nullptr) && (m_currentGm == client))
    {
        findNewGM();
    }

    emit itemChanged();
    return (0 < i);
}

void Channel::clearData()
{
    m_dataToSend.clear();
    m_memorySize = 0;
    emit memorySizeChanged(m_memorySize,this);
}
bool Channel::removeChild(TreeItem* itm)
{
    if(itm->isLeaf())
    {
        removeClient(static_cast<TcpClient*>(itm));
        return true;
    }
    else
    {
        if(itm->childCount() == 0)
        {
            m_child.removeAll(itm);
            return true;
        }
    }
    return false;
}
bool Channel::hasNoClient()
{
    bool hasNoClient = true;
    for(auto child : m_child)
    {
        if(child->isLeaf())
        {
            hasNoClient = false;
        }
    }
    return hasNoClient;
}

void Channel::sendOffGmStatus(TcpClient* client)
{
    bool isRealGM = false;
    if(m_currentGm == nullptr || m_currentGm == client)
    {
        m_currentGm = client;
        isRealGM = true;
    }
    if(nullptr == client)
        return;

    NetworkMessageWriter* message = new NetworkMessageWriter(NetMsg::AdministrationCategory, NetMsg::GMStatus);
    QStringList idList;
    idList << client->getPlayerId();
    message->setRecipientList(idList,NetworkMessage::OneOrMany);
    message->int8(static_cast<qint8>(isRealGM));
    sendToMany(message,nullptr);
}
void Channel::findNewGM()
{
    auto result = std::find_if(m_child.begin(), m_child.end(),[=](TreeItem*& item){
        auto client = dynamic_cast<TcpClient*>(item);
        if(nullptr != client)
        {
            if(client->isGM())
            {
                return true;
            }
        }
        return false;
    });

    if(result == m_child.end())
        m_currentGm = nullptr;
    else
        m_currentGm = dynamic_cast<TcpClient*>(*result);

    sendOffGmStatus(m_currentGm);
}
bool Channel::isCurrentGm(TreeItem* item)
{
    return (m_currentGm == item);
}

QString Channel::getCurrentGmId()
{
    if(m_currentGm)
        return m_currentGm->getPlayerId();
    return {};
}
