export interface ScaleData {
  ID: number;
  CreatedOn: number;
  ScaleID: Scale;
  Value: number;
}

export enum Scale {
  Container,
  Plate,
}
