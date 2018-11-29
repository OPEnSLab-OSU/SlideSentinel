function [varargout] = ImportFile2Vectors_standalone(varargin)
    %IMPORTFILE2TABLE(filename,delimiter,HeaderLine,DataStartLine)
    %  Imports data from the specified file and outputs vectors. Ignores
    %  empty columns. If no header (header row=0), vector names will be var1,
    %  var2, etc.
    %
    %   Inputs (can get from running function or will be prompted) - if 
    %           inputting, put in this order with placeholders ('~'):
    %       filename(optional): path and name of file as a string
    %       delimiter(optional): column delimiter as a string or character
    %       HeaderLine(optional): row to find header on (start at 1, use 0 for no header)
    %       DataStartLine(optional): first row with data (start at 1)
    %
    %   Outputs:
    %      varargout: data imported into individual vectors
    %
    % Example: [time,x,y,z,int,temp] = ImportFile2Vectors_standalone('~',',',1,1);
    %   prompt for file name, comma delimeter, header in 1st line, data in
    %       2nd line, 6 output variables
    %
    %   Created by Cara Walter
    %   Modified 11/26/2018
    
    %% Get or assign input variables
    % At least 1 variable entered
    if ~isempty(varargin)
        if strcmp(varargin{1},'~')
            %Prompt for the input file if not input
            [InputFileName,InputFileFolder]=uigetfile('*.*','Input file');
            filename = [InputFileFolder,InputFileName];
        else
            % assign input to variable
            filename = varargin{1};
        end

        if numel(varargin)==1
            % Prompt for delimiter if not input
            delimiter=inputdlg('Input file delimiter as a word or character');

            % Prompt for row to look for header in if not input
            HeaderLine=str2double(inputdlg('Input number for row to find header in, start at 1'));

            % prompt for row to start looking for data in if not input
            startRow=str2double(inputdlg('Input number for first row with data, start at 1'));
            
        % only 2 variables input
        elseif numel(varargin)==2
            % assign input to variable
            delimiter=varargin{2};
            
            % Prompt for row to look for header in if not input
            HeaderLine=str2double(inputdlg('Input number for row to find header in, start at 1'));

            % prompt for row to start looking for data in if not input
            startRow=str2double(inputdlg('Input number for first row with data, start at 1'));
            
        % only 3 variables input
        elseif numel(varargin)==3
            % 3 variables input, but placeholder for 2nd
            if strcmp(varargin{2},'~')
                 % Prompt for delimiter if not input
                delimiter=inputdlg('Input file delimiter as a word or character');
            else
                % assign input to variable
                delimiter=varargin{2};
            end
            % assign input to variable
            HeaderLine=varargin{3};
            
            % prompt for row to start looking for data in if not input
            startRow=str2double(inputdlg('Input number for first row with data, start at 1'));
        else
             if strcmp(varargin(2),'~')
                 % Prompt for delimiter if not input
                 delimiter=inputdlg('Input file delimiter as a word or character');
             else
                 % assign input to variable
                 delimiter=varargin{2};
             end
            
             if strcmp(varargin{3},'~')
                % Prompt for row to look for header in if not input
                HeaderLine=str2double(inputdlg('Input number for row to find header in, start at 1'));
             else
                 % assign input to variable
                HeaderLine=varargin{3};
             end
             % assign input to variable
             startRow=varargin{4};
        end
        
    % Nothing input when running function
    else
        %Prompt for the input file if not input
        [InputFileName,InputFileFolder]=uigetfile('*.*','Input file');
        filename = [InputFileFolder,InputFileName];

        % Prompt for delimiter if not input
        delimiter=inputdlg('Input file delimiter as a word or character');

        % Prompt for row to look for header in if not input
        HeaderLine=str2double(inputdlg('Input number for row to find header in, start at 1'));

        % prompt for row to start looking for data in if not input
        startRow=str2double(inputdlg('Input number for first row with data, start at 1'));
    end
    
    % parse delimiter
    if strcmpi(delimiter,'tab') || strcmp(delimiter,char(9))
        Delim=char(9);
    elseif strcmpi(delimiter,'comma') || strcmp(delimiter,',')
        Delim=',';
    elseif strcmpi(delimiter,'semi-colon') || strcmp(delimiter,';')
        Delim=';';
    elseif strcmpi(delimiter,'space') || strcmp(delimiter,char(32))
        Delim=char(32);
    end
    
    if ischar(HeaderLine)
        HeaderLine=str2double(HeaderLine);
    end
    
    if ischar(startRow)
        startRow=str2double(startRow);
    end
    
    %% Start processing file
    % create file identifier 
    fid=fopen(filename,'r');
    
    if HeaderLine>0
        % read in lines until get to headerline
        for i=1:HeaderLine
            lineIn=fgetl(fid);
        end
        fprintf('Header: %s \n',lineIn(1:end-1));

        % separate header into cells
        %% StringParse
        %indices for delimiter
        idx=find(lineIn==Delim); 
        %allocate memory
        Header=cell(1,numel(idx)+1);
        %split LineIn into pieces by delimiter
        LastIndex=0;
        for i=1:(numel(idx)+1)
            %Use variable to include text after last delimiter
            if i==numel(idx)+1
                NewIndex=numel(lineIn)+1;
                Header{i}=lineIn(LastIndex+1:NewIndex-1);
            else
                NewIndex=idx(i);        
                %case of empty between delimiters
                if idx(i)-LastIndex==1
                    Header{i}='';
                else
                    Header{i}=lineIn(LastIndex+1:NewIndex-1); 
                end
            %Use variable to allow inclusion of text before 1st delimiter
            LastIndex=idx(i);
            end
        end

        % delete empty cells
        Header = Header(~cellfun('isempty',Header));

        % Remove last column if the header is empty
        if strcmp(Header(end),'')
            Header=Header(1:end-1);
            minuscol=1;
        else
            minuscol=0;
        end

        % remove empty cells and periods
        %Header = Header(~cellfun('isempty',Header));
        Header = cellfun(@(x) strrep(x,'.','_'),Header,'UniformOutput',false);

        % Adjust header to remove spaces and change duplicates
        HeaderAdj=genvarname(Header);

    else
        % read in first data line
        for i=1:startRow
            lineIn=fgetl(fid);
        end
        disp('No header');
        %indices for delimiter
        idx=find(lineIn==Delim); 
        HeaderAdj = cell(1,numel(idx)+1);
        for v = 1:numel(idx+1)
            HeaderAdj{v} = ['var' num2str(v)];
        end
        minuscol=0;
    end
    
    formatSpec='';
    % specify formatting
    for i=1:numel(HeaderAdj)
        formatSpec=[formatSpec '%s']; %#ok<AGROW>
    end
    formatSpec=[formatSpec '%[^\n\r]'];

    % put file back at beginning
    frewind(fid);
    
    %% read file into array
    dataArray = textscan(fid, formatSpec, 'Delimiter', Delim,...
         'TreatAsEmpty',{'NA','na','NAN','NaN','nan'},...
         'HeaderLines' ,startRow, 'ReturnOnError', false);
    
    if minuscol
        dataArray=dataArray(:,1:end-1);
    end
    
    % close the file
    fclose(fid); 
    
    % allocate memory
    textCols=nan(1,numel(HeaderAdj));
    dataCols=nan(1,numel(HeaderAdj));
    tc=0;
    dc=0;
        
    % determine data type for each row - number vs. string and convert
    % numbers
    for i=1:numel(HeaderAdj)
        tr=0;
        dr=0;
        rawData=dataArray{:,i};
        % code originally generated by Matlab from file importer
        for row=1:size(rawData, 1)
            % Create a regular expression to detect and remove non-numeric 
            % prefixes and suffixes.
            regexstr = '(?<prefix>.*?)(?<numbers>([-]*(\d+[\,]*)+[\.]{0,1}\d*[eEdD]{0,1}[-+]*\d*[i]{0,1})|([-]*(\d+[\,]*)*[\.]{1,1}\d+[eEdD]{0,1}[-+]*\d*[i]{0,1}))(?<suffix>.*)';
            % creates cell array with a structure of prefix, numbers,
            % suffix for each entry - empty if a string
            result = regexp(rawData{row}, regexstr, 'names');
            % if a number
            if ~isempty(result)
                % if there is nothing in prefix or suffix
                if isempty(result.prefix) && isempty(result.suffix)
                    % get just the numbers
                    numbers = result.numbers;

                    % Detected commas in non-thousand locations.
                    invalidThousandsSeparator = false;
                    if any(numbers==',')
                        thousandsRegExp = '^\d+?(\,\d{3})*\.{0,1}\d*$';
                        if isempty(regexp(thousandsRegExp, ',', 'once'))
                            numbers = NaN;
                            invalidThousandsSeparator = true;
                        end
                    end
                    % Convert numeric strings to numbers.
                    if ~invalidThousandsSeparator
                        numbers = textscan(strrep(numbers, ',', ''), '%f');
                        % put numeric version back in array
                        dataArray{row, i} = numbers{1};
                    end
                    % tally row as data
                    dr=dr+1;
                else
                    % tally row as text
                    tr=tr+1;
                    dataArray{row,i}=rawData{row};
                end
            % if the original cell is empty, count as data
            elseif isempty(rawData{row})
                dr=dr+1;
                dataArray{row, i} = NaN;
            else
                % tally row as text
                tr=tr+1;
                dataArray{row,i}=rawData{row};
            end
        end
        % if there are less data rows than the total number of rows, assign
        % as text column
        if dr<row
            tc=tc+1;
            textCols(tc)=i;
        % otherwise assign as data column
        else
            dc=dc+1;
            dataCols(dc)=i;
        end
    end
    
    % reduce column variables to just used
    % Split data into numeric and cell columns.
    if tc>0
        textCols=textCols(1:tc);
        rawCellColumns = dataArray(:, textCols);
    else
        rawCellColumns={};
    end
    if dc>0
        dataCols=dataCols(1:dc);
        rawNumericColumns = dataArray(:, dataCols);
        % Replace non-numeric cells with NaN (may be redundant)
        R = cellfun(@(x) ~isnumeric(x) && ~islogical(x),rawNumericColumns); % Find non-numeric cells
        rawNumericColumns(R) = {NaN}; % Replace non-numeric cells
    else
        rawNumericColumns=[];
    end
    
    %% Create output variable
    tc=0;
    dc=0;
    for i=1:numel(HeaderAdj)
        % if the current column is a data column convert it
        if ~isempty(find(dataCols==i, 1)) && ~isempty(rawNumericColumns)
            dc=dc+1;
            varargout{i}=cell2mat(rawNumericColumns(:, dc));
        % if the current column is a text column, just transfer it
        elseif ~isempty(find(textCols==i,1)) && ~isempty(rawCellColumns)
            tc=tc+1;
            varargout{i}=rawCellColumns(:, tc);
        end
    end
    disp('Processing complete')
end
    
