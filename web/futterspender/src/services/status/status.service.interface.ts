import { Subject } from 'rxjs';
import { MachineStatus } from 'src/models/MachineStatus';

export interface IStatusService {
  MachineStatus: Subject<MachineStatus | null>;
  Connected: Subject<boolean>;
  Loading: Subject<boolean>;
  SetupMode: Subject<boolean>;
  espMessage: Subject<string>;
  fetchMachineStatus(): void;
  setFetchInterval(seconds: number): void;
  startFeed(): void;
  stopFeed(): void;
}
