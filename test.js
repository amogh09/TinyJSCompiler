var person = function (firstName, lastName, age, eyeColor) {
    this.firstName = firstName;
    this.lastName = lastName;
    this.age = age;
    this.eyeColor = eyeColor;
    try{
      this.changeName = function (name) {
          this.lastName = name;
      };
    } catch(err) {
      this.lastName = "error";
    }

}

var p = new person("Rajvi", "Nar", 21, "brown");
p.changeName("Rathore");
p.lastName
