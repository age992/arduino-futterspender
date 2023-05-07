import { Subject } from 'rxjs';
import { FetchInterval } from 'src/models/FetchInterval';
import { MachineStatus } from 'src/models/MachineStatus';

export interface IStatusService {
  MachineStatus: Subject<MachineStatus | null>;
  Connected: Subject<boolean>;
  Loading: Subject<boolean>;
  SetupMode: Subject<boolean>;
  fetchMachineStatus(): void;
  setFetching(interval: FetchInterval): void;
  startFeed(): void;
  stopFeed(): void;
}
