import { StatusMockService } from 'src/services/status/status.mock.service';
import { StatusService } from 'src/services/status/status.service';

export const environment = {
  production: false,
  providers: [{ provide: StatusService, useClass: StatusMockService }],
};
