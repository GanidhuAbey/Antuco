- how ".pass" files work

- name signifies the name of the pass, the framebuilder hashes this name to generate an id, so make sure it is unique.

- resources is used to signify an array of resources that the currently defined pass will require access from. its important to specify
  here so that both framebuilder and currently defined pass can access/manipulate the resource as required.