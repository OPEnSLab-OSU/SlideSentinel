# SatComm Functionality Overview

The SatComm implementation is split into two state machines: SatCommController and SatCommDriver. SatCommController is designed to only handle business logic, dispatching events to SatCommDriver to handle hardware interactions. This split allows the business logic to be unit tested separately from hardware interactions, which are notoriously difficult to test with. These diagrams can be viewed using [PlantUML](https://plantuml.com/).

For more information on state machines and testing, please see [SatCommArchitecture](../../SatCommArchitecture.md).

## SatComm Controller

```plantuml
@startuml
skinparam defaultFontName Verdana
hide empty description

[*] --> WaitToTX
state WaitToIdle
state Connected {
    WaitToTX -left-> TXRX : DriverReady
    state CheckQueue <<choice>>
    CheckQueue --> TXRX : [else]
    TXRX --> CheckQueue : Any Success
    Idle --> TXRX : RingAlert
    Idle -right-> WaitToIdle : SignalLost
    WaitToIdle -left-> Idle : DriverReady
    CheckQueue --> Idle : QueueEmpty
    TXRX --> WaitToTX : SignalLost
    WaitToIdle -up-> WaitToTX : RingAlert
}

TXRX : entry / StartRecieve or StartSendReceive

note right of Connected
connected() will evaluate to true here
end note

note right of WaitToIdle
Contains a Queue for both
inbound and outbound packets,
the decision checks to see
if the outbound queue is empty.
end note
@enduml
```

## SatCommDriver
```plantuml
@startuml
skinparam defaultFontName Verdana
hide empty description

[*] -right-> Off
state PowerUpChoice <<choice>>
Off --> PowerUpChoice : PowerUp
PowerUpChoice --> Wait : Success
PowerUpChoice --> Off
Wait --> Ready : Signal
Ready --> Wait : Failure or No Signal

Off : PowerUp / Initialize RockBlock
Off : Failure / Panic
Wait : entry / SignalLost
Wait : Update / Check Signal
Ready : entry / SyncTime
Ready : entry / DriverReady
Ready : entry / Attatch Ring Alert Interrupt
Ready : exit / Dettach Ring Alert Interrupt
Ready : Update / Check Signal and RingAlert
Ready : StartSendRecieve / Send/Recieve Packet, any Success or SignalLost
Ready : StartRecieve / Recieve Packet, any Success or SignalLost

note left of PowerUpChoice 
Checks if we can 
communicate
with the RockBlock
endnote
@enduml
```