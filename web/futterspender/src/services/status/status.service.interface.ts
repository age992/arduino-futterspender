import { Subject } from 'rxjs';
import { MachineStatus } from 'src/models/MachineStatus';

export interface IStatusService {
  MachineStatus: Subject<MachineStatus | null>;
  Connected: Subject<boolean>;
  Loading: Subject<boolean>;
  update(): void;
  startFeed(): void;
  stopFeed(): void;
}
