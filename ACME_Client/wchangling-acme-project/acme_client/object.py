from typing import Set

class ACMEOrder:
   def __init__(self, location: str, orderObj: dict) -> None:
      # TODO: parse returned order object
      self.location = location
      self.domainNames: set[str] = set([i['value'] for i in orderObj['identifiers']])
      self.authorizations: list[str] = orderObj['authorizations']
      self.finalize: str = orderObj['finalize']
      # self.challenges:list[(str, str)] = []
      
