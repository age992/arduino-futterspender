import { StatusService } from 'src/services/status/status.service';

export const environment = {
  production: true,
  providers: [{ provide: StatusService, useClass: StatusService }],
  apiUrl: location.origin,
};
