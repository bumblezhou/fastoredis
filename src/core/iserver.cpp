#include "core/iserver.h"

#include <QApplication>

#include "common/qt/convert_string.h"
#include "common/net/net.h"

#include "core/logger.h"

namespace
{
    using namespace fastoredis;

    struct connectFunct
    {
        explicit connectFunct(bool isConnect)
            : isConnect_(isConnect)
        {

        }

        bool operator()(const QObject *sender, const char *signal, const QObject *receiver, const char *member, Qt::ConnectionType type) const
        {
            if(isConnect_){
                return QObject::connect(sender, signal, receiver, member, type);
            }
            else{
                return QObject::disconnect(sender, signal, receiver, member);
            }
        }

        const bool isConnect_;
    };

    void syncServersFunct(IServer *src, IServer *dsc, const connectFunct &func)
    {
        VERIFY(func(src, SIGNAL(startedConnect(const EventsInfo::ConnectInfoRequest &)), dsc, SIGNAL(startedConnect(const EventsInfo::ConnectInfoRequest &)), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(finishedConnect(const EventsInfo::ConnectInfoResponce &)), dsc, SIGNAL(finishedConnect(const EventsInfo::ConnectInfoResponce &)), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(startedDisconnect(const EventsInfo::DisonnectInfoRequest &)), dsc, SIGNAL(startedDisconnect(const EventsInfo::DisonnectInfoRequest &)), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(finishedDisconnect(const EventsInfo::DisConnectInfoResponce &)), dsc, SIGNAL(finishedDisconnect(const EventsInfo::DisConnectInfoResponce &)), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(startedExecute(const EventsInfo::ExecuteInfoRequest &)), dsc, SIGNAL(startedExecute(const EventsInfo::ExecuteInfoRequest &)), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(finishedExecute(const EventsInfo::ExecuteInfoResponce &)), dsc, SIGNAL(finishedExecute(const EventsInfo::ExecuteInfoResponce &)), Qt::UniqueConnection));

        VERIFY(func(src, SIGNAL(rootCreated(const EventsInfo::CommandRootCreatedInfo& )), dsc, SIGNAL(rootCreated(const EventsInfo::CommandRootCreatedInfo& )), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(rootCompleated(const EventsInfo::CommandRootCompleatedInfo& )), dsc, SIGNAL(rootCompleated(const EventsInfo::CommandRootCompleatedInfo& )), Qt::UniqueConnection));

        VERIFY(func(src, SIGNAL(addedChild(FastoObject *)), dsc, SIGNAL(addedChild(FastoObject *)), Qt::UniqueConnection));
        VERIFY(func(src, SIGNAL(itemUpdated(FastoObject*, const QString&)), dsc, SIGNAL(itemUpdated(FastoObject*, const QString&)), Qt::UniqueConnection));
   }
}

namespace fastoredis
{
    IServer::IServer(const IDriverSPtr &drv, bool isMaster)
        : drv_(drv), isMaster_(isMaster)
    {
        if(isMaster_){
            VERIFY(QObject::connect(drv_.get(), SIGNAL(addedChild(FastoObject *)), this, SIGNAL(addedChild(FastoObject *))));
            VERIFY(QObject::connect(drv_.get(), SIGNAL(itemUpdated(FastoObject*, const QString&)), this, SIGNAL(itemUpdated(FastoObject*, const QString&))));
        }
    }

    void IServer::unSyncServers(IServer *src, IServer *dsc)
    {
        static const connectFunct f(false);
        syncServersFunct(src, dsc, f);
        syncServersFunct(dsc, src, f);
    }

    void IServer::syncServers(IServer *src, IServer *dsc)
    {
        static const connectFunct f(true);
        syncServersFunct(src, dsc, f);
        syncServersFunct(dsc, src, f);
    }

    IDriverSPtr IServer::driver() const
    {
        return drv_;
    }

    QString IServer::name() const
    {
        return common::convertFromString<QString>(drv_->settings()->connectionName());
    }

    QString IServer::address() const
    {
        std::string shost = common::convertToString(drv_->address());
        return common::convertFromString<QString>(shost);
    }

    QString IServer::outputDelemitr() const
    {
        return common::convertFromString<QString>(drv_->outputDelemitr());
    }

    connectionTypes IServer::connectionType() const
    {
        return drv_->connectionType();
    }

    void IServer::connect()
    {
        EventsInfo::ConnectInfoRequest req;
        emit startedConnect(req);
        QEvent *ev = new Events::ConnectRequestEvent(this, req);
        notify(ev);
    }

    void IServer::shutDown()
    {
        EventsInfo::ShutDownInfoRequest req;
        emit startedShutdown(req);
        QEvent *ev = new Events::ShutDownRequestEvent(this, req);
        notify(ev);
    }

    void IServer::backupToPath(const QString& path)
    {
        EventsInfo::BackupInfoRequest req(common::convertToString(path));
        emit startedBackup(req);
        QEvent *ev = new Events::BackupRequestEvent(this, req);
        notify(ev);
    }

    void IServer::exportFromPath(const QString& path)
    {
        EventsInfo::ExportInfoRequest req(common::convertToString(path));
        emit startedExport(req);
        QEvent *ev = new Events::ExportRequestEvent(this, req);
        notify(ev);
    }

    void IServer::loadDatabases()
    {
        EventsInfo::LoadDatabasesInfoRequest req;
        emit startedLoadDatabases(req);
        QEvent *ev = new Events::LoadDatabasesInfoRequestEvent(this, req);
        notify(ev);
    }

    void IServer::loadDatabaseContent(const DataBaseInfo &inf)
    {
        EventsInfo::LoadDatabasesContentRequest req(inf);
        emit startedLoadDataBaseContent(req);
        QEvent *ev = new Events::LoadDatabaseContentRequestEvent(this, req);
        notify(ev);
    }

    void IServer::disconnect()
    {
        EventsInfo::DisonnectInfoRequest req;
        emit startedDisconnect(req);
        QEvent *ev = new Events::DisconnectRequestEvent(this, req);
        notify(ev);
    }

    void IServer::execute(const QString &script)
    {
        EventsInfo::ExecuteInfoRequest req(common::convertToString(script));
        emit startedExecute(req);
        QEvent *ev = new Events::ExecuteRequestEvent(this, req);
        notify(ev);
    }

    void IServer::serverInfo()
    {
        EventsInfo::ServerInfoRequest req;
        emit startedLoadServerInfo(req);
        QEvent *ev = new Events::ServerInfoRequestEvent(this, req);
        notify(ev);
    }

    void IServer::serverProperty()
    {
        EventsInfo::ServerPropertyInfoRequest req;
        emit startedLoadServerProperty(req);
        QEvent *ev = new Events::ServerPropertyInfoRequestEvent(this, req);
        notify(ev);
    }

    void IServer::changeProperty(const PropertyType &newValue)
    {
        EventsInfo::ChangeServerPropertyInfoRequest req;
        req.newItem_ = newValue;
        emit startedChangeServerProperty(req);
        QEvent *ev = new Events::ChangeServerPropertyInfoRequestEvent(this, req);
        notify(ev);
    }

    void IServer::requestHistoryInfo()
    {
        EventsInfo::ServerInfoHistoryRequest req;
        emit startedLoadServerHistoryInfo(req);
        QEvent *ev = new Events::ServerInfoHistoryRequestEvent(this, req);
        notify(ev);
    }

    void IServer::loadServerInfoHistoryEvent(Events::ServerInfoHistoryResponceEvent *ev)
    {
        using namespace Events;
        ServerInfoHistoryResponceEvent::value_type v = ev->value();
        common::ErrorValueSPtr er = v.errorInfo();
        if(er && er->isError()){
            LOG_ERROR(er, true);
        }
        emit finishedLoadServerHistoryInfo(v);
    }

    void IServer::processConfigArgs()
    {
        EventsInfo::ProcessConfigArgsInfoRequest req;
        QEvent *ev = new Events::ProcessConfigArgsRequestEvent(this, req);
        notify(ev);
    }

    void IServer::stopCurrentEvent()
    {
        drv_->interrupt();
    }

    bool IServer::isConnected() const
    {
        return drv_->isConnected();
    }

    bool IServer::isMaster() const
    {
        return isMaster_;
    }

    void IServer::setIsMaster(bool isMaster)
    {
        isMaster_ = isMaster;
    }

    bool IServer::isLocalHost() const
    {
        return common::net::isLocalHost(drv_->address().host_);
    }

    IServer::~IServer()
    {
    }

    void IServer::notify(QEvent *ev)
    {
        emit progressChanged(0);
        qApp->postEvent(drv_.get(), ev);
    }

    void IServer::customEvent(QEvent *event)
    {
        using namespace Events;
        QEvent::Type type = event->type();
        if (type == static_cast<QEvent::Type>(ConnectResponceEvent::EventType)){
            ConnectResponceEvent *ev = static_cast<ConnectResponceEvent*>(event);
            connectEvent(ev);

            ConnectResponceEvent::value_type v = ev->value();
            common::ErrorValueSPtr er(v.errorInfo());
            if(!er){
                processConfigArgs();
            }
        }
        else if(type == static_cast<QEvent::Type>(EnterModeEvent::EventType))
        {
            EnterModeEvent *ev = static_cast<EnterModeEvent*>(event);
            EnterModeEvent::value_type v = ev->value();
            emit enteredMode(v);
        }
        else if(type == static_cast<QEvent::Type>(LeaveModeEvent::EventType))
        {
            LeaveModeEvent *ev = static_cast<LeaveModeEvent*>(event);
            LeaveModeEvent::value_type v = ev->value();
            emit leavedMode(v);
        }
        else if(type == static_cast<QEvent::Type>(CommandRootCreatedEvent::EventType)){
            CommandRootCreatedEvent *ev = static_cast<CommandRootCreatedEvent*>(event);
            CommandRootCreatedEvent::value_type v = ev->value();
            emit rootCreated(v);
        }
        else if(type == static_cast<QEvent::Type>(CommandRootCompleatedEvent::EventType)){
            CommandRootCompleatedEvent *ev = static_cast<CommandRootCompleatedEvent*>(event);
            CommandRootCompleatedEvent::value_type v = ev->value();
            emit rootCompleated(v);
        }
        else if(type == static_cast<QEvent::Type>(DisconnectResponceEvent::EventType))
        {
            DisconnectResponceEvent *ev = static_cast<DisconnectResponceEvent*>(event);
            disconnectEvent(ev);
        }
        else if(type == static_cast<QEvent::Type>(LoadDatabasesInfoResponceEvent::EventType))
        {
            LoadDatabasesInfoResponceEvent *ev = static_cast<LoadDatabasesInfoResponceEvent*>(event);
            loadDatabaseInfosEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(LoadDatabaseContentResponceEvent::EventType)){
            LoadDatabaseContentResponceEvent *ev = static_cast<LoadDatabaseContentResponceEvent*>(event);
            loadDatabaseContentEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(ServerInfoResponceEvent::EventType)){
            ServerInfoResponceEvent *ev = static_cast<ServerInfoResponceEvent*>(event);
            loadServerInfoEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(ServerInfoHistoryResponceEvent::EventType)){
            ServerInfoHistoryResponceEvent *ev = static_cast<ServerInfoHistoryResponceEvent*>(event);
            loadServerInfoHistoryEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(ServerPropertyInfoResponceEvent::EventType)){
            ServerPropertyInfoResponceEvent *ev = static_cast<ServerPropertyInfoResponceEvent*>(event);
            loadServerPropertyEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(ChangeServerPropertyInfoResponceEvent::EventType)){
            ChangeServerPropertyInfoResponceEvent *ev = static_cast<ChangeServerPropertyInfoResponceEvent*>(event);
            serverPropertyChangeEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(BackupResponceEvent::EventType)){
            BackupResponceEvent *ev = static_cast<BackupResponceEvent*>(event);
            handleBackupEvent(ev);
        }
        else if (type == static_cast<QEvent::Type>(ExportResponceEvent::EventType)){
            ExportResponceEvent *ev = static_cast<ExportResponceEvent*>(event);
            handleExportEvent(ev);
        }
        else if(type == static_cast<QEvent::Type>(ProgressResponceEvent::EventType))
        {
            ProgressResponceEvent *ev = static_cast<ProgressResponceEvent*>(event);
            ProgressResponceEvent::value_type v = ev->value();
            emit progressChanged(v);
        }
        return QObject::customEvent(event);
    }
}
