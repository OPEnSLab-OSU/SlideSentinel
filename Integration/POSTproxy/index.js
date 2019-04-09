/*Name: index.js
 *Author: Kevin Koos
 *Description: client side js for index.html
 */

//checks that the forms fields are filled with something
function checkFields() {
    var type = $('input[name=type]:checked').val();
    var form = $('#' + type + '-form > :input[type=text]');
    //check form text inputs
    for (var i = 0; i < form.length; i++) {
        if (form[i].value == "") {
            return false;
        }
    }
    //check the password input
    if ($('#password').val() == '') {
        return false;
    }
    
    return true;
}

//regex check for a valid url string
function validURL(str) {
  var pattern = new RegExp('^(https?:\\/\\/)?'+ // protocol
    '((([a-z\\d]([a-z\\d-]*[a-z\\d])*)\\.)+[a-z]{2,}|'+ // domain name
    '((\\d{1,3}\\.){3}\\d{1,3}))'+ // OR ip (v4) address
    '(\\:\\d+)?(\\/[-a-z\\d%_.~+]*)*'+ // port and path
    '(\\?[;&a-z\\d%_.~+=-]*)?'+ // query string
    '(\\#[-a-z\\d_]*)?$','i'); // fragment locator
  return !!pattern.test(str);
}

$(document).ready(function() {
    
    //jquery-ui set up
    $('input[type=radio').checkboxradio();

    $('input:text, input:password')
      .button()
      .css({
              'font' : 'inherit',
             'color' : 'inherit',
        'text-align' : 'left',
           'outline' : 'none',
            'cursor' : 'text'
     });    
    
    //prevent default enter key submission
    $(window).keydown(function(event){
        if(event.keyCode == 13) {
            event.preventDefault();
            return false;
        }
    });
    
    //hide or show form elements based of type of input requested
    $('#main-type').change(function() {
        switch($("input[name=type]:checked").val()) {
            case "new":
                $('#new-form').find('input[type=text], textarea').val('');
                $('#password').val('');
                $('#new-form').show();
                $('#existing-form').hide();
                $('#remove-form').hide();
                break;
                
            case "existing":
                $('#existing-form').find('input[type=text], textarea').val('');
                $('#password').val('');
                $('#new-form').hide();
                $('#existing-form').show();
                $('#remove-form').hide();
                break;
                
            case "remove":
                $('#remove-form').find('input[type=text], textarea').val('');
                $('#password').val('');
                $('#new-form').hide();
                $('#existing-form').hide();
                $('#remove-form').show();
                break;
        }
        
        $('#res-body').html('');
        
    });
    
    //send a post request back to server with special id and info
    $('#submit').click(function() {            
            //check that fields are filled
            if (checkFields()) {
                
                var type = $('input[name=type]:checked').val();
                var post_data = $('#' + type + '-form').serialize();
                post_data = post_data.concat('&pass=' + $('#password').val());
                var url = window.location.origin + '/?id=' + type.toUpperCase();
                var user_url = null;
                
                //if not remove, get the url the user inputted
                if (type != 'remove') {
                    user_url = post_data.match(/url=([^&]+)/)[1];
                    //serialize encodes unsafe ascii character, must undo
                    user_url = decodeURIComponent(user_url);
                }
                
                //check valid url if not remove or short vircuit out if it is
                if (type == 'remove' || (type != 'remove' && validURL(user_url))) {
                    
                    //clear responses and post fields to server
                    $('#res-body').html('');

                    $.get(url, post_data, function(data) {
                        //put the response in the resp body
                        $('#res-body').html(data);
                    });
                    
                //url failed check
                } else {
                    $('#res-body').html('INVALID URL ENTERED');
                }   
                    
            //fields not filled
            } else {
                $('#res-body').html('ALL FIELDS NOT FILLED IN');
            }
            
    });
    
});
