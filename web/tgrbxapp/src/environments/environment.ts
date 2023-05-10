import { StatusService } from 'src/services/status/status.service';

export const environment = {
  production: true,
  providers: [],
  apiUrl: location.origin + '/api',
  wsUrl: location.origin + '/ws',
};
