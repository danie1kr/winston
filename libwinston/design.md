

# API

## Rail

### traverse(entry) -> exit
When starting at entry, exit returns where the track leads to

### switch()
Trigger a switch

### attachSignal(signal, exit)
Attach a signal to guard exit

## SignalTower

### order(command)
Put command into work queue

### work()
Process work queue

### updateSignals(switch)
Traverse switch exits and put light the signals

## Railway

### validate -> bool
True, if the railway tracks are connected properly

## Digital Central Station

## Signal

### set(state)
Update signal lights

## Locomotive