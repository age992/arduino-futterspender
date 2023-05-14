export interface MachineStatus {
  ContainerLoad: number;
  PlateLoad: number;
  Open: boolean;
  MotorOperation: boolean;
  SDCardConnection: boolean;
  AutomaticFeeding: boolean;
  ManualFeeding: boolean;
}
