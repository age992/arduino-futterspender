import { Injectable } from '@angular/core';
import { IStatusService } from './status.service.interface';
import { MachineStatus } from 'src/models/MachineStatus';
import { BehaviorSubject, ReplaySubject, Subject } from 'rxjs';
import { FetchInterval } from 'src/models/FetchInterval';

@Injectable({
  providedIn: 'root',
})
export class StatusMockService implements IStatusService {
  public MachineStatus = new BehaviorSubject<MachineStatus | null>(null);

  private machineStatusInternal: MachineStatus | null = null;

  public Connected = new BehaviorSubject(true);
  public Loading = new BehaviorSubject(false);
  public SetupMode = new BehaviorSubject(false);

  private lastUpdate = 0;
  private changeStatusDelay = 20000;
  private updateInterval = 10000;

  constructor() {
    this.fetchMachineStatus();
    setInterval(this.fetchMachineStatus, this.updateInterval);
  }

  setFetching(interval: FetchInterval): void {
    throw new Error('Method not implemented.');
  }

  private notify = () => {
    this.MachineStatus.next(this.machineStatusInternal);
  };

  public fetchMachineStatus = () => {
    console.log('Update status...');
    this.Loading.next(true);

    setTimeout(() => {
      const now = new Date().getTime();
      if (now - this.lastUpdate >= this.changeStatusDelay) {
        const newMachineStatus: MachineStatus = {
          ContainerLoad: Math.floor(Math.random() * 1000 * 100) / 100,
          PlateLoad: Math.floor(Math.random() * 10 * 100) / 100,
          MotorOperation: true,
          SDCardConnection: true,
          Open: false,
        };
        this.lastUpdate = now;
        this.machineStatusInternal = newMachineStatus;
        this.notify();
      }

      this.Loading.next(false);
    }, 1000);
  };

  startFeed = () => {
    console.log('Start feeding...');
  };

  stopFeed = () => {
    console.log('...feeding stopped.');
  };
}
