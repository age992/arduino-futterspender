import { Injectable } from '@angular/core';
import { IStatusService } from './status.service.interface';
import { BehaviorSubject, Subject } from 'rxjs';
import { MachineStatus } from 'src/models/MachineStatus';

@Injectable({
  providedIn: 'root',
})
export class StatusService implements IStatusService {
  public MachineStatus: Subject<MachineStatus | null> = new Subject();
  public Connected = new BehaviorSubject(true);
  public Loading = new BehaviorSubject(false);

  constructor() {}

  update(): void {
    throw new Error('Method not implemented.');
  }

  startFeed = () => {
    throw new Error('Method not implemented.');
  };

  stopFeed = () => {
    throw new Error('Method not implemented.');
  };
}
